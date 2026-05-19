#include <cstring>
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <stdlib.h>
#endif

void print_free_memory() {
    #ifdef _WIN32
        // גישת Windows: שימוש ב-Windows API ומבני נתונים של ה-Kernel
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);

        if (GlobalMemoryStatusEx(&statex)) {
            printf("Windows System:\n");
            printf("Free physical memory: %llu MB\n", statex.ullAvailPhys / 1024 / 1024);
        } else {
            printf("Failed to get memory status.\n");
        }

    #else
        // גישת Linux: קריאת קובץ "מדומה" מתוך מערכת הקבצים /proc
        printf("Linux System (Reading from /proc/meminfo):\n");
        
        FILE *fp = fopen("/proc/meminfo", "r");
        if (fp == NULL) {
            perror("Failed to open /proc/meminfo");
            return;
        }

        char line[256];
        // אנחנו מחפשים את השורה שמתחילה ב-MemFree
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "MemFree:", 8) == 0) {
                printf("%s", line); // מדפיס את השורה כפי שהיא מופיעה בקובץ
                break;
            }
        }
        fclose(fp);
    #endif
}

int main() {
    print_free_memory();
    return 0;
}
