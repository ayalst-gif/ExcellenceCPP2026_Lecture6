#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>

// --- Logic for Child 1 (Writer to Pipe) ---
void run_child1(int pipe_write_fd) {
    std::string input_line;
    
    std::cout << "Child 1 (PID: " << getpid() << "): Enter text to send through pipeline:\n";
    std::cout << "> " << std::flush;
    
    if (std::getline(std::cin, input_line)) {
        std::cout << "Child 1 echoed: " << input_line << std::endl;
        
        input_line += "\n";
        
        if (write(pipe_write_fd, input_line.c_str(), input_line.length()) == -1) {
            perror("Child 1: write to pipe failed");
            exit(EXIT_FAILURE);
        }
    }
    
    std::cout << "Child 1 (PID: " << getpid() << ") is closing its write FD number: " << pipe_write_fd << std::endl;
    close(pipe_write_fd);
    exit(EXIT_SUCCESS);
}

// --- Logic for Child 2 (Reader from Pipe) ---
void run_child2(int pipe_read_fd) {
    char buffer[256];
    std::memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_read = read(pipe_read_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        std::cout << "Child 2 (PID: " << getpid() << ") received: " << buffer;
    } else if (bytes_read == -1) {
        perror("Child 2: read from pipe failed");
        exit(EXIT_FAILURE);
    }
    
    std::cout << "Child 2 (PID: " << getpid() << ") is closing its read FD number: " << pipe_read_fd << std::endl;
    close(pipe_read_fd);
    exit(EXIT_SUCCESS);
}

// --- Coordinator (Parent Process) ---
int main() {
    int pipe_fds[2];
    
    if (pipe(pipe_fds) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    // Print the opened Pipe FD IDs immediately after allocation
    std::cout << "Parent: Pipe created successfully.\n";
    std::cout << "  -> Read End FD ID: " << pipe_fds[0] << "\n";
    std::cout << "  -> Write End FD ID: " << pipe_fds[1] << "\n\n";

    // Fork Child 1
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("Failed to fork Child 1");
        return 1;
    } 
    else if (pid1 == 0) {
        // Inside Child 1
        std::cout << "Child 1 (PID: " << getpid() << ") is closing unused read FD number: " << pipe_fds[0] << std::endl;
        close(pipe_fds[0]); 
        run_child1(pipe_fds[1]);
    }

    // Fork Child 2
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("Failed to fork Child 2");
        return 1;
    } 
    else if (pid2 == 0) {
        // Inside Child 2
        std::cout << "Child 2 (PID: " << getpid() << ") is closing unused write FD number: " << pipe_fds[1] << std::endl;
        close(pipe_fds[1]); 
        run_child2(pipe_fds[0]);
    }

    // --- Inside Parent ---
    std::cout << "Parent: Closing its own copies of FDs (" << pipe_fds[0] << " and " << pipe_fds[1] << ")\n";
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    // Clean up both zombie processes safely
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);

    std::cout << "\nParent: Both children terminated successfully. Pipeline destroyed.\n";
    return 0;
}
