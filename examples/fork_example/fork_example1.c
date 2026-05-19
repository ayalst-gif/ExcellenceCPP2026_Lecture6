#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    } 
    else if (pid == 0) {
        // קוד זה רץ רק בתהליך הבן
        printf("I am the CHILD (PID: %d). My parent is %d\n", getpid(), getppid());
    } 
    else {
        // קוד זה רץ רק בתהליך האב
        printf("I am the PARENT (PID: %d). My child is %d\n", getpid(), pid);
        wait(NULL); // מחכה שהבן יסיים
    }
    return 0;
}
