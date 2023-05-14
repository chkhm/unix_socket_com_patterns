#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

// required for POLLRDHUP macro
#define _GNU_SOURCE 
#include <poll.h>

#define BUF_SIZE 100

#define SV_SOCK_PATH "/tmp/mysock"
// 100 ms timeout for poll
#define TIMEOUT 100
#define BACKLOG 5

#define PORT 8080


enum domain_mode_t {
    unix_domain_mode,
    tcp_domain_mode
};

struct socket_data_t {
        int socket_family = AF_UNIX;
        int socket_type = SOCK_STREAM;
        int protocol = 0;
};

socket_data_t data_types[] = {
    { AF_UNIX, SOCK_STREAM, 0},
    { AF_INET, SOCK_STREAM, 0}
};

int make_socket(domain_mode_t mode) {
    int s = socket(data_types[mode].socket_family, data_types[mode].protocol, data_types[mode].protocol);
    return s;
}

int make_bind_unix_domain(int sfd) {
    struct sockaddr_un addr;
    if (strlen(SV_SOCK_PATH) > sizeof(addr.sun_path) - 1) {
        printf("path to socket too long");
        return -1;
    }
    // ENOENT means "does not exist"
    if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT) {
        printf("Couldn't remove socket file");
        return -1;
    }
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    int rslt = bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un));
    if (rslt == -1) {
        printf("failed to bind to socket.");
        return -1;
    }
    return rslt;
}

int make_bind_tcp_domain(int sfd) {
    struct sockaddr_in addr;
    int opt = 1;
    
    // avoid "address already in use error"
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        // exit(EXIT_FAILURE);
        return -1;
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
  
    int rslt = bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    if (rslt < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    return rslt;
}

int make_bind(domain_mode_t mode, int sfd) {
    switch (mode) {
        case unix_domain_mode:
        return make_bind_unix_domain(sfd);
        case tcp_domain_mode:
        return make_bind_tcp_domain(sfd);
    }
    return -1;
}

int main(int argc, char *argv[]) {
    domain_mode_t mode = unix_domain_mode;
    if (argc > 1) {
        if (argv[1][1] == 'u' || argv[1][1] == 'U') {
            mode = unix_domain_mode;
        } else if (argv[1][1] == 't' || argv[1][1] == 'T') {
            mode = tcp_domain_mode;
        } else {
            printf("Wrong parameter %s", argv[1]);
            return -1;
        }
    }

    int sfd = make_socket(mode);
    printf("Server socket fd = %d\n", sfd);
    if (sfd == -1) {
        printf("create socket failed");
        return -1;
    }

    int rslt = make_bind(mode, sfd);
    if (rslt < 0) {
        printf("binding failed.");
        return -1;
    }

    if (listen(sfd, BACKLOG) == -1) {
        printf("failed to listen to socket.");
        return -1;
    }

    ssize_t numRead;
    char buf[BUF_SIZE];

    // passive server socket plus up to 99 client sockets
    struct pollfd sockets[100];
    int num_sockets = 1;

    sockets[0].fd = sfd;
    sockets[0].events = POLLIN;
    sockets[0].revents = 0;

    // init fd with -1 means ignore for the poll command
    for (int i = 1; i<100; i++) {
        sockets[i].fd = -1;
        sockets[i].events = 0;
        sockets[i].revents = 0;
    }

    for (;;) {

        int poll_rslt = poll(sockets, num_sockets, TIMEOUT);
        if (poll_rslt < 0) {
            printf("Error during poll: %d: %s", errno, strerror(errno));
            return -1;
        }
        if (poll_rslt == 0) {
            continue;
        }

        if (sockets[0].revents !=0 && sockets[0].revents != POLLIN) {
            printf("Error on server socket: %d: %s\n", errno, strerror(errno));
            return -1;
        }
        if (sockets[0].revents == POLLIN) {
            printf("Waiting to accept a connection...\n");
            // NOTE: blocks until a connection request arrives. But we got a poll event, so should return right away.
            int cfd = accept(sfd, NULL, NULL);
            printf("Accepted socket fd = %d\n", cfd);
            sockets[num_sockets].fd = cfd;
            sockets[num_sockets].events = POLLIN;
            sockets[num_sockets].revents = 0;
            num_sockets++;
        }

        for (int i=1; i < 100; i++) {
            if (sockets[i].revents != 0 && sockets[i].revents != POLLIN ) {
                printf("Error on client socket[%d]: %d: %s\n", i, errno, strerror(errno));
                close(sockets[i].fd);
                sockets[i].fd = -1;
                sockets[i].events = 0;
                sockets[i].revents = 0;
            }
            else if (sockets[i].revents == POLLIN) {
                int read_bytes = 0;
                while((read_bytes = read(sockets[i].fd, buf, BUF_SIZE)) > 0) {
                    buf[read_bytes] = '\0';
                    printf("read from socket %d : %s\n", i, buf);
                    int write_bytes = write(sockets[i].fd, buf, read_bytes);
                    if (write_bytes != read_bytes) {
                        printf("write to socket[%d] only partial/ incomplete.", i);
                    }
                }
                if (read_bytes < 0) {
                    printf("error reading from socket[%d].", i);
                    close(sockets[i].fd);
                    sockets[i].fd = -1;
                    sockets[i].events = 0;
                    sockets[i].revents = 0;
                }
            }
        }
    } // end forever loop

    return 0;
}