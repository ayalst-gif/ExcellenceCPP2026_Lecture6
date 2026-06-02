#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>
#include <atomic>

std::atomic<bool> keep_running{true};

void signal_handler(int signum) {
    keep_running = false;
}

// --- Child 1: Receives data via UNIX Socket ---
void run_child1(int socket_fd) {
    std::signal(SIGINT, signal_handler);
    char ch;
    std::string current_message;

    std::cout << "Child 1 (PID: " << getpid() << ") listening via UNIX Domain Socket...\n";

    while (keep_running && read(socket_fd, &ch, 1) > 0) {
        if (ch == '\n') {
            std::cout << "Child 1 [UNIX Socket] received: " << current_message << std::endl;
            current_message.clear();
        } else {
            current_message += ch;
        }
    }

    std::cout << "Child 1 (PID: " << getpid() << ") is closing UNIX Socket FD: " << socket_fd << std::endl;
    close(socket_fd);
    exit(EXIT_SUCCESS);
}

// --- Child 2: Receives data via TCP Network Socket ---
void run_child2(int socket_fd) {
    std::signal(SIGINT, signal_handler);
    char ch;
    std::string current_message;

    std::cout << "Child 2 (PID: " << getpid() << ") listening via TCP Network Socket...\n";

    while (keep_running && read(socket_fd, &ch, 1) > 0) {
        if (ch == '\n') {
            std::cout << "Child 2 [TCP Network] received: " << current_message << std::endl;
            current_message.clear();
        } else {
            current_message += ch;
        }
    }

    std::cout << "Child 2 (PID: " << getpid() << ") is closing Network Socket FD: " << socket_fd << std::endl;
    close(socket_fd);
    exit(EXIT_SUCCESS);
}

int main() {
    std::signal(SIGINT, signal_handler);

    // =================================================================
    // 1. SETUP UNIX DOMAIN SOCKET
    // =================================================================
    int unix_fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, unix_fds) == -1) {
        perror("UNIX socketpair creation failed");
        return 1;
    }

    // =================================================================
    // 2. SETUP INTERNET TCP SOCKET (Loopback)
    // =================================================================
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) { perror("Internet socket creation failed"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    server_addr.sin_port = htons(0); // Random ephemeral port selection

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed"); return 1;
    }
    if (listen(server_fd, 1) == -1) { perror("Listen failed"); return 1; }

    socklen_t len = sizeof(server_addr);
    getsockname(server_fd, (struct sockaddr*)&server_addr, &len);
    int allocated_port = ntohs(server_addr.sin_port);

    // =================================================================
    // 3. FORK CHILD 1 (UNIX Endpoint Receiver)
    // =================================================================
    pid_t pid1 = fork();
    if (pid1 == 0) {
        close(unix_fds[0]); // Child 1 doesn't need Parent's side of UNIX socket
        close(server_fd);   // Child 1 doesn't need TCP listener socket
        run_child1(unix_fds[1]);
    }
    close(unix_fds[1]); // Parent safely closes child's endpoint copy

    // =================================================================
    // 4. FORK CHILD 2 (TCP Endpoint Receiver)
    // =================================================================
    pid_t pid2 = fork();
    if (pid2 == 0) {
        close(unix_fds[0]); // Child 2 has no relation to the UNIX socket pair
        
        int client_fd = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_port = htons(allocated_port);
        
        while (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
            usleep(10000); 
        }
        close(server_fd); 
        run_child2(client_fd);
    }

    // Parent completes the TCP handshake with Child 2
    int child2_net_fd = accept(server_fd, nullptr, nullptr);
    close(server_fd); // The TCP listener can be safely closed now

    // =================================================================
    // 5. MAIN PARENT ROUTING LOOP (Reading from STDIN)
    // =================================================================
    std::string input_line;
    std::cout << "\n================================================================\n";
    std::cout << "Parent Process is ready. Type messages below to broadcast to both children.\n";
    std::cout << "Type 'exit' or use Ctrl+C to terminate the demonstration safely.\n";
    std::cout << "================================================================\n";

    while (keep_running) {
        std::cout << "\nParent STDIN > " << std::flush;
        if (!std::getline(std::cin, input_line) || input_line == "exit") {
            break;
        }
        if (input_line.empty()) continue;

        input_line += "\n"; // Inject newline frame boundary

        // Broadcast path A: Write out to Child 1 (UNIX Domain Socket)
        if (write(unix_fds[0], input_line.c_str(), input_line.length()) == -1) {
            perror("Parent failed writing to UNIX socket");
            break;
        }

        // Broadcast path B: Write out to Child 2 (TCP Network Socket)
        if (write(child2_net_fd, input_line.c_str(), input_line.length()) == -1) {
            perror("Parent failed writing to TCP socket");
            break;
        }
    }

    // =================================================================
    // 6. TEARDOWN AND SANITIZATION
    // =================================================================
    std::cout << "\nParent: STDIN processing loop broke. Shutting down system sockets...\n";
    close(unix_fds[0]);
    close(child2_net_fd);

    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

    std::cout << "Parent: Children reclaimed successfully. Process tree cleanup finished.\n";
    return 0;
}

