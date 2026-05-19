#include <iostream>
#include <unistd.h>   // fork, kill, getpid
#include <sys/wait.h> // wait
#include <signal.h>   // SIGKILL
#include <chrono>     // std::chrono
#include <thread>     // std::this_thread::sleep_for

// --- Child Process Logic ---
void run_child_logic() {
    int counter = 1;
    while (true) {
        // Child prints a message every second
        std::cout << "Child process: message " << counter++ 
                  << " (PID: " << getpid() << ")" << std::endl;
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// --- Parent Process Logic ---
void run_parent_logic(pid_t child_pid) {
    // Parent sleeps for 4 seconds while child is running
    std::this_thread::sleep_for(std::chrono::seconds(4));
    
    std::cout << "\nParent: Decided to kill the child process..." << std::endl;
    
    // Sending SIGKILL to the child
    if (kill(child_pid, SIGKILL) == 0) {
        std::cout << "Parent: Sent SIGKILL to child." << std::endl;
    }

    std::cout << "Parent: Waiting for child to be reaped (wait)..." << std::endl;
    wait(nullptr);
    
    std::cout << "Parent: Child is dead. Terminating program." << std::endl;
}

int main() {
    // Forking the process
    if (pid_t pid = fork(); pid == -1) {
        perror("fork failed");
        return 1;
    } 
    else if (pid == 0) {
        // We are inside the child process
        run_child_logic();
    } 
    else {
        // We are inside the parent process
        run_parent_logic(pid);
    }

    return 0;
}
