#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>

#define PORT "1234"
#define BACKLOG 5

void sigchld_handler(int s) {
    (void)s;

    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main() {
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results of the getaddrinfo call
    struct addrinfo *p, *q; // iterates thru servinfo
    char ipstr[INET6_ADDRSTRLEN]; // buffer to hold the ip address string for printing
    int sockfd, new_fd; // file descriptor? will be used to return from socket call
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    int yes = 1;
    struct sigaction sa;

    memset(&hints, 0, sizeof hints); // make sure struct 'hints' is empty
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets, as opposed to datagram
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    for (p = servinfo; p !=NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break; // both calls successful
    }

    freeaddrinfo(servinfo); // free the linked list

    assert(p != NULL);

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {
        addr_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), ipstr, sizeof ipstr);
        printf("server: got connection from %s\n", ipstr);

        if (!fork()) {
            close(sockfd);
            char msg[] = "HTTP/1.0 200 OK\r\nContent-Length: 12\r\n\r\nHello World!";
            int len = strlen(msg);
            if (send(new_fd, msg, len, 0) == -1) {
                perror("send");
            }
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }
    return 0;
}