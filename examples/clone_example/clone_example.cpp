#include <iostream>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>
#include <string.h>

// 1. Shared Memory Variable
int shared_value = 100;

// 2. Shared File Descriptor variable
int global_fd = -1;

// Function for the cloned child process
int child_logic(void* arg) {
    auto label = static_cast<const char*>(arg);
    std::cout << "Child [" << label << "]: Started. shared_value = " << shared_value << std::endl;

    // Modify shared memory
    shared_value += 50;
    std::cout << "Child [" << label << "]: Updated shared_value to " << shared_value << std::endl;

    // Work on the file if it exists in memory
    if (global_fd != -1) {
        const char* child_msg = "Message from Child: Writing to the shared FD.\n";
        write(global_fd, child_msg, strlen(child_msg));
        
        std::cout << "Child [" << label << "]: Closing the file descriptor now..." << std::endl;
        close(global_fd);
    }
    
    return 0;
}

void run_full_demo(int flags, const std::string& description) {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "Demo: " << description << std::endl;
    std::cout << "==========================================" << std::endl;

    // Reset shared state
    shared_value = 100;
    
    // 1. Parent opens the file
    global_fd = open("out.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (global_fd == -1) {
        perror("Parent: Failed to open out.txt");
        return;
    }
    
    std::cout << "Parent: Opened out.txt (FD: " << global_fd << "). Writing initial line..." << std::endl;
    const char* parent_text = "Parent: This is the starting line.\n";
    write(global_fd, parent_text, strlen(parent_text));

    // 2. Prepare child stack
    const int STACK_SIZE = 64 * 1024;
    std::vector<char> stack(STACK_SIZE);
    char* stack_ptr = stack.data() + STACK_SIZE;

    // 3. Clone the child
    pid_t pid = clone(child_logic, stack_ptr, flags | SIGCHLD, (void*)description.c_str());

    if (pid == -1) {
        perror("clone failed");
        close(global_fd);
        return;
    }

    // 4. Wait for child to finish
    waitpid(pid, nullptr, 0);

    // 5. Final Checks in Parent
    std::cout << "\n--- Parent Post-Child Checks ---" << std::endl;
    std::cout << "Parent: shared_value is now: " << shared_value << " (" 
              << (shared_value == 150 ? "Shared" : "Private") << ")" << std::endl;

    std::cout << "Parent: Checking if FD " << global_fd << " is still valid..." << std::endl;
    
    // Check if FD is still open using fcntl
    if (fcntl(global_fd, F_GETFD) == -1) {
        std::cout << "Parent Result: FD is CLOSED. (Shared File Table)" << std::endl;
    } else {
        std::cout << "Parent Result: FD is still OPEN. (Private File Table)" << std::endl;
        
        const char* final_msg = "Parent: Still here, writing the final line.\n";
        write(global_fd, final_msg, strlen(final_msg));
        close(global_fd);
    }

    // Display actual file content
    std::cout << "\nActual file content of out.txt:" << std::endl;
    std::cout << "-------------------------------" << std::endl;
    system("cat out.txt");
    std::cout << "-------------------------------" << std::endl;
}

int main() {
    // Scenario 1: Private Memory & Private Files (Like a standard fork)
    run_full_demo(0, "FORK-LIKE (No flags)");

    // Scenario 2: Shared Memory ONLY
    run_full_demo(CLONE_VM, "CLONE_VM (Shared Memory, Private Files)");

    // Scenario 3: Shared Memory AND Shared Files (Like a Thread)
    run_full_demo(CLONE_VM | CLONE_FILES, "CLONE_VM + CLONE_FILES (Full Sharing)");

    return 0;
}
