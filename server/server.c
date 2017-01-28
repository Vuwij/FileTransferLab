#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include "../constants.h"

#define BACKLOG 10  // how many pending connections queue will hold
#define MAXBUFLEN 100

void usage(char *program) {
    fprintf(stderr, "Usage: %s <UDP listen port>\n", program);
    exit(1);
}

/** get socket address, IPv4 or IPv6: */
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int port;

    //Parsing command line arguments
    if (argc != 2) usage(argv[0]);

    port = atoi(argv[1]);
    if (!(0 <= port && port <= 65535)) {
        fprintf(stderr, "port = %d should be within 0-65535\n", port);
        usage(argv[0]);
    }

    /**************************************************************************
     *                            Section 1                                   *
     **************************************************************************/

    // 1. Open a UDP socket and listen at the specified port number
    /* 2. Receive a message from the client
     *      If msg is "ftp" reply "yes"
     *      else reply "no"     */

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET for IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // Using my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // looping through all results and bind the first one available
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }

    freeaddrinfo(servinfo);
    printf("server: waiting to recvfrom...\n");
    addr_len = sizeof their_addr;

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    buf[numbytes] = '\0';

    char *reply = (strcmp(buf, "ftp") == 0) ? "yes" : "no";


    printf("listener: got packet from %s\n",
            inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *) &their_addr),
            s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);
    printf("listener: sending \"%s\"\n", reply);

    if ((numbytes = sendto(sockfd, reply, strlen(reply), 0, (struct sockaddr *) &their_addr, addr_len)) == -1) {
        perror("sendto");
        exit(1);
    }

    /**************************************************************************
     *                            Section 2                                   *
     **************************************************************************/






    /**************************************************************************
     *                            Section 3                                   *
     **************************************************************************/
    // Upon receiving the first packet in a sequence (frag_no = 1)
    // The program should read the file name from the packet and create a corresponding
    // file stream on the local file system
    // Data read from packets should then be written to this file stream
    // If the EOF packet is received, the file stream should be closed

    char pacHeader[2000];
    struct packet pac;

    if ((numbytes = recvfrom(sockfd, pacHeader, sizeof (pacHeader), 0,
            (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }

    // Using tokenizer
    const char delim[2] = ":";
    char *token;

    // First argument
    token = strtok(pacHeader, delim);
    pac.total_frag = (unsigned int) strtoul(token, NULL, 10);

    // Second argument
    token = strtok(NULL, delim);
    pac.frag_no = (unsigned int) strtoul(token, NULL, 10);

    // Third argument
    token = strtok(NULL, delim);
    pac.size = (unsigned int) strtoul(token, NULL, 10);

    // Fourth argument
    pac.filename = strtok(NULL, delim);

    // Fifth argument

    // Testing
    printf("%d\n %d\n %d\n %s\n", pac.total_frag, pac.frag_no, pac.size, pac.filename);


    // Make or open the file
    FILE *fp = fopen(pac.filename, "a");

    close(fileno(fp));
    close(sockfd);
    return 0;
}
