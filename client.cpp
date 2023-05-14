
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>

#define BUF_SIZE 100
#define SV_SOCK_PATH "/tmp/mysock"
// 100 ms timeout for poll
#define TIMEOUT 100
#define BACKLOG 5


int main(int argc, char *argv[]) {
    struct sockaddr_un addr;
    ssize_t numRead;
    char buf[BUF_SIZE];

    if (argc < 2) {
        printf("need one command line parameter (which is the string to be sent to the server)");
        return -1;
    }

    // Create a new client socket with domain: AF_UNIX, type: SOCK_STREAM, protocol: 0
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    printf("Client socket fd = %d\n", sfd);

    if (sfd == -1) {
        printf("create socket failed");
        return -1;
    }

    if (strlen(SV_SOCK_PATH) > sizeof(addr.sun_path) - 1) {
        printf("path to socket too long");
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    printf("connecting to: %s\n", SV_SOCK_PATH);
    if (connect(sfd, (struct sockaddr *) &addr,
                sizeof(struct sockaddr_un)) == -1) {
      printf("Error on conncet: %d %s", errno, strerror(errno));
      return -1;
    }
    printf ("connected.\n");

    int len = strlen(argv[1]);

    for (int n = 0; n < 10; n++) {
        printf("writing: %s\n", argv[1]);
        if (write(sfd, argv[1], len) != len) {
            printf("error on write: partial incomplete write");
            return -1;
        }
        int read_bytes = read(sfd, buf, BUF_SIZE);
        if (read_bytes < len) {
            printf("read error.");
            return -1;
        }
        buf[read_bytes] = '\0';
        printf("received: %s\n", buf);
        sleep(5);
    }
    close(sfd);
    return 0;
}