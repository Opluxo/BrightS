#include <stdio.h>
#include <unistd.h>

int main(void) {
    printf("hello from dynamic ARM64 binary\n");
    printf("pid=%d\n", getpid());
    printf("uid=%d\n", getuid());
    fflush(stdout);
    return 0;
}
