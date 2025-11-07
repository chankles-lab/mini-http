#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT "1234"
#define BACKLOG 5

int main() {
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results of the getaddrinfo call
    struct addrinfo *p, *q; // iterates thru servinfo
    char ipstr[INET6_ADDRSTRLEN]; // buffer to hold the ip address string for printing
    int sockfd, new_fd; // file descriptor? will be used to return from socket call
    int bind_status;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;

    memset(&hints, 0, sizeof hints); // make sure struct 'hints' is empty
    hints.ai_family = AF_UNSPEC; // don't care ip4 or ip6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets, as opposed to datagram
    hints.ai_flags = AI_PASSIVE; // fill in my IP for me

    if ((status = getaddrinfo(NULL, "1234", &hints, &servinfo)) != 0) {
        fprintf(stderr, "gai error: %s\n", gai_strerror(status));
        exit(1);
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos

    // ... do everything until you don't need servinfo anymore ... 
    printf("IP addresses for localhost\n\n");
    for(p = servinfo; p != NULL; p = p->ai_next) {
        void *addr;
        char *ipver;
        struct sockaddr_in *ipv4;
        struct sockaddr_in6 *ipv6;

        // get the pointer to the address itself,
        // different fields in IPv4 and IPv6:
        if (p->ai_family == AF_INET) { // IPv4
            ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &(ipv4->sin_addr);
            ipver = "IPv4";
        } else { // IPv6
            ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &(ipv6->sin6_addr);
            ipver = "IPv6";
        }

        // convert the IP to a string and print it:
        inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
        printf("  %s: %s\n", ipver, ipstr);
    }

    printf("Binding to socket\n");
    for (q = servinfo; q !=NULL; q = q->ai_next) {
        sockfd = socket(q->ai_family, q->ai_socktype, q->ai_protocol);
        if (sockfd == -1) {
            perror("server: socket");
            continue;
        }

        bind_status = bind(sockfd, q->ai_addr, q->ai_addrlen);
        if (bind_status == -1) {
            perror("server: bind");
            continue;
        }

        break; // both calls successful
    }
    printf("Successfully returned sockfd: %d\n", sockfd);
    printf("Successfully binded to socket: %d\n", bind_status);

    freeaddrinfo(servinfo); // free the linked list

    int listen_status = listen(sockfd, BACKLOG);
    if (listen_status == -1) {
        perror("server: listen");
        exit(1);
    }

    addr_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
    close(sockfd);
    if (new_fd == -1) {
        perror("server: accept");
        exit(1);
    }

    char *msg = "Hello...can you hear me?\n";
    int len, bytes_sent;

    len = strlen(msg);
    bytes_sent = send(new_fd, msg, len, 0);

    close(new_fd);
    return 0;
}