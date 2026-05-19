#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    char ch;
    int bytes_read;
    
    // פתיחת הקבצים
    int fd_read = open("text.txt", O_RDONLY);
    int fd_append = open("text.txt", O_WRONLY | O_APPEND);

    if (fd_read == -1 || fd_append == -1) {
        perror("Error opening file");
        return 1;
    }

    // 1. קריאה ראשונית עד סוף הקובץ
    printf("--- Phase 1: fd_read - Reading until EOF ---\n");
    while (read(fd_read, &ch, 1) > 0) {
        putchar(ch);
    }
    printf("\n(fd_read - Reached EOF)\n");

    // 2. בדיקה: האם יש תוכן חדש לפני שכתבנו?
    printf("\n--- Phase 2: Checking for new data BEFORE writing ---\n");
    if ((bytes_read = read(fd_read, &ch, 1)) > 0) {
        printf("Content added: %c\n", ch);
    } else {
        printf("FD was not added with new data\n");
    }

    // 3. כתיבה לסוף הקובץ
    printf("\n--- Phase 3: Writing 'SEVEN' via FD_APPEND ---\n");
    const char *msg = "SEVEN\n";
    write(fd_append, msg, strlen(msg));

    // 4. בדיקה: האם יש תוכן חדש אחרי הכתיבה?
    printf("\n--- Phase 4: Checking for new data AFTER writing ---\n");
    
    // ננסה לקרוא שוב מאותו fd_read שקודם היה ב-EOF
    if ((bytes_read = read(fd_read, &ch, 1)) > 0) {
        printf("Content added: %c", ch); // מדפיס את התו הראשון שנקרא (S)
        // קריאת שאר התוכן שנוסף
        while (read(fd_read, &ch, 1) > 0) {
            putchar(ch);
        }
        printf("\n");
    } else {
        printf("FD was not added with new data\n");
    }

    close(fd_read);
    close(fd_append);
    return 0;
}
