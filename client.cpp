#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define BUF_SIZE 100
#define SV_SOCK_PATH "/tmp/mysock"
// 100 ms timeout for poll
#define TIMEOUT 100
#define BACKLOG 5

#define PORT 8080


enum domain_mode_t {
    unix_domain_mode = 0,
    tcp_domain_mode = 1
};

int make_socket(domain_mode_t mode) {
    printf("make socket ");
    printf("mode: %d\n", mode);
    int s = -1;
    switch (mode) {
        case unix_domain_mode:
            s = socket(AF_UNIX, SOCK_STREAM, 0);
            break;
        case tcp_domain_mode:
            s = socket(AF_INET, SOCK_STREAM, 0);
            break;
    }
    printf("socket is %d\n", s);
    return s;
}

int make_connect_unix(int sfd) {
    struct sockaddr_un addr;

    if (strlen(SV_SOCK_PATH) > sizeof(addr.sun_path) - 1) {
        printf("path to socket file too long");
        return -1;
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    printf("connecting to: %s\n", SV_SOCK_PATH);
    int rslt = connect(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (rslt == -1) {
      printf("Error on conncet: %d %s", errno, strerror(errno));
      return -1;
    }
    printf ("connected.\n");
    return rslt;
}

int make_connect_tcp(int sfd) {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
  
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr) <= 0) {
        printf(
            "nslookup on addr failed.\n");
        return -1;
    }
    int rslt = connect(sfd, (struct sockaddr*)&addr, sizeof(addr));
    if (rslt < 0) {
        printf("\nconnect failed \n");
        return -1;
    }
    return rslt;
}

int make_connect(int mode, int sfd) {
    int rslt = -1;
    switch (mode) {
        case unix_domain_mode:
        rslt = make_connect_unix(sfd);
        break;
        case tcp_domain_mode:
        rslt = make_connect_tcp(sfd);
        break;
    }
    return rslt;
}


int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("need connect type and one command line parameter (which is the string to be sent to the server)");
        return -1;
    }

    domain_mode_t mode = unix_domain_mode;
    if (argv[1][0] == 'u' || argv[1][0] == 'U') {
        mode = unix_domain_mode;
    } else if (argv[1][0] == 't' || argv[1][0] == 'T') {
        mode = tcp_domain_mode;
    } else {
        printf("Wrong parameter %s", argv[1]);
        return -1;
    }

    char *msg= argv[2];
    int len = strlen(msg);

    struct sockaddr_un addr;
    ssize_t numRead;
    char buf[BUF_SIZE];


    // Create a new client socket with domain: AF_UNIX, type: SOCK_STREAM, protocol: 0
    int sfd = make_socket(mode);
    printf("Client socket fd = %d\n", sfd);

    if (sfd == -1) {
        printf("create socket failed");
        return -1;
    }

    int rslt = make_connect(mode, sfd);
    if (rslt < 0) {
        return -1;
    }

    for (int n = 0; n < 10; n++) {
        printf("writing: %s\n", msg);
        if (write(sfd, msg, len) != len) {
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