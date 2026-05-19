#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// הוספנו const לפני ה-char* כדי לציין שהפונקציה לא תשנה את המחרוזת
void run_child(const char *path) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
    } else if (pid == 0) {
        // execv עדיין מצפה למערך של char* (לא const)
        // לכן נבצע המרה קטנה (casting) רק עבור הארגומנטים
        char *args[] = {(char *)path, NULL};
        execv(args[0], args);
        
        perror("Execv failed");
        exit(1);
    } else {
    	printf("Main process: Created child process %d",pid);
        wait(NULL);
    }
}

int main() {
    int choice;
    while (1) {
        printf("\n--- Menu ---\n");
        printf("1. Print Triangle\n");
        printf("2. Print Rectangle\n");
        printf("3. Exit\n");
        printf("Select option: ");
        
        if (scanf("%d", &choice) != 1) {
            // ניקוי הקלט במקרה של טעות
            while(getchar() != '\n'); 
            continue;
        }

        if (choice == 1) run_child("./print_triangle");
        else if (choice == 2) run_child("./print_rectangle");
        else if (choice == 3) break;
        else printf("Invalid option!\n");
    }
    return 0;
}
