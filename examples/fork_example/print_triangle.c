#include <stdio.h>
#include <unistd.h>

int main() {
    usleep(500000); // המתנה של חצי שניה לפני הכל
    printf("PROCESS %d starts\n", getpid());

    printf("\n--- Triangle ---\n");
    for (int i = 1; i <= 5; i++) {
        for (int j = 1; j <= i; j++) printf("*");
        printf("\n");
    }

    usleep(500000); // המתנה נוספת לפני הסיום
    printf("PROCESS %d ends\n", getpid());
    return 0;
}
