#include <stdio.h>
#include <unistd.h>

int main() {
    usleep(500000); // המתנה של חצי שניה לפני הכל
    printf("PROCESS %d starts\n", getpid());

    printf("\n--- Rectangle ---\n");
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 10; j++) printf("*");
        printf("\n");
    }

    usleep(500000); // המתנה נוספת לפני הסיום
    printf("PROCESS %d ends\n", getpid());
    return 0;
}
