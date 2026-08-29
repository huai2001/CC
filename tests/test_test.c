#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
    const char* path = getenv("ANDROID_HOME");
    printf("PATH: %s\n", path);
    return 0;
}
