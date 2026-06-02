#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <csignal>
#include <atomic>

// Global atomic flag to safely communicate across signal handlers
std::atomic<bool> keep_running{true};

// Signal handler function for SIGINT (Ctrl+C)
void signal_handler(int signum) {
    // Standard I/O in signal handlers isn't technically "async-signal-safe",
    // but for demo purposes, we notify the user we caught it.
    keep_running = false;
}

// --- Logic for Child 1 (Continuous Writer) ---
void run_child1(int pipe_write_fd) {
    std::string input_line;
    
    // Register the same signal handler inside the child
    std::signal(SIGINT, signal_handler);

    std::cout << "Child 1 (PID: " << getpid() << ") started. Type 'exit' or press Ctrl+C to stop.\n";

    while (keep_running) {
        std::cout << "\nChild 1 > " << std::flush;
        
        if (!std::getline(std::cin, input_line)) {
            // End of stream (e.g., Ctrl+D)
            break;
        }

        // Check for exit command
        if (input_line == "exit") {
            std::cout << "Child 1 detected 'exit' command.\n";
            break;
        }

        if (input_line.empty()) continue;

        std::cout << "Child 1 echoed: " << input_line << std::endl;
        input_line += "\n"; // Append newline delimiter for Child 2
        
        if (write(pipe_write_fd, input_line.c_str(), input_line.length()) == -1) {
            perror("Child 1: write to pipe failed");
            break;
        }
    }
    
    std::cout << "Child 1 (PID: " << getpid() << ") is closing write FD: " << pipe_write_fd << std::endl;
    close(pipe_write_fd);
    exit(EXIT_SUCCESS);
}

// --- Logic for Child 2 (Continuous Reader) ---
void run_child2(int pipe_read_fd) {
    // Register the signal handler so it terminates cleanly on Ctrl+C
    std::signal(SIGINT, signal_handler);

    char ch;
    std::string current_message;

    // Read char-by-char to safely isolate lines sent from Child 1
    // read() will naturally return 0 (EOF) when Child 1 closes its write FD
    while (keep_running && read(pipe_read_fd, &ch, 1) > 0) {
        if (ch == '\n') {
            std::cout << "Child 2 (PID: " << getpid() << ") received line: " << current_message << std::endl;
            current_message.clear();
        } else {
            current_message += ch;
        }
    }
    
    std::cout << "Child 2 (PID: " << getpid() << ") is closing read FD: " << pipe_read_fd << std::endl;
    close(pipe_read_fd);
    exit(EXIT_SUCCESS);
}

// --- Coordinator (Parent Process) ---
int main() {
    // Register signal handler in the parent process
    std::signal(SIGINT, signal_handler);

    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    std::cout << "Parent: Pipe created. Read FD: " << pipe_fds[0] << ", Write FD: " << pipe_fds[1] << "\n\n";

    // Fork Child 1
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Failed to fork Child 1");
        return 1;
    } 
    else if (pid1 == 0) {
        close(pipe_fds[0]); // Close unused read end
        run_child1(pipe_fds[1]);
    }

    // Fork Child 2
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("Failed to fork Child 2");
        return 1;
    } 
    else if (pid2 == 0) {
        close(pipe_fds[1]); // Close unused write end
        run_child2(pipe_fds[0]);
    }

    // Inside Parent: Close global copies immediately
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    std::cout << "Parent: Waiting for children to finish processing...\n";

    // Wait for both children to exit cleanly
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

    std::cout << "\nParent: Both children clean. Continuous pipeline safely torn down.\n";
    return 0;
}
