#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include "../constants.h"

#define MAXDATASIZE 100 // max number of bytes we can get at once
// get sockaddr, IPv4 or IPv6:
#define MAXBUFSIZE 100

void usage(char *program) {
    fprintf(stderr, "Usage: %s <server address> <server port number>\n", program);
    exit(1);
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int port;
    char serverAddress[MAXBUFSIZE];

    //Parsing command line arguments
    if (argc != 3) usage(argv[0]);

    strncpy(serverAddress, argv[1], MAXBUFSIZE);

    port = atoi(argv[2]);
    if (!(0 <= port && port <= 65535)) {
        fprintf(stderr, "port = %d should be within 0-65535\n", port);
        usage(argv[0]);
    }

    /**************************************************************************
     *                            Section 1                                   *
     **************************************************************************/

    // 1. Ask the user to input a message follows the format: ftp <file name>
    char str[MAXBUFSIZE];
    char protocolType[MAXBUFSIZE];
    char fileName[MAXBUFSIZE];

    while (1) {
        scanf("%s %s", protocolType, fileName);
        if (strcmp(protocolType, "ftp") != 0)
            fprintf(stderr, "Usage: ftp <file name>\n");
        else
            break;
    }

    // 2. Check the existence of the file:
    //      a) if exists, send a message "ftp" to the server
    //      b) else, exit

    // Open a file stream
    FILE *fp = fopen(fileName, "r");

    // Check if it exists
    if (!fp) {
        fprintf(stderr, "Error: the file does not exist in the folder\n");
        exit(1);
    }
    
    // Make a network connection and find the correct socket
    int sockfd;                                                         // Socket File Descriptor
    struct addrinfo hints, *servinfo, *p;                               // Address information of the Connection
    int rv;                                                             // Return value of the socket
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;                                        // Unspecified Address Family
    hints.ai_socktype = SOCK_STREAM;                                    // Socket type stream (TCP)
    if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));         // gai_strerror -> Get Address Info String Error
        return EXIT_FAILURE;
    }
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),   // inet_ntop -> IP Network Network to Presentation
            s, sizeof s);
    printf("client: connecting to %s\n", s);
    
    // Sending a message "ftp" to the server
    if (send(sockfd, "ftp", 13, 0) == -1)
        perror("send");

    // 3. Receive a message from the server:
    //      a) If the message is "yes", print out "A file transfer can start"
    //      b) else, exit
    char msg[MAXBUFSIZE];
    int numbytes;

    if ((numbytes = recv(sockfd, msg, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    msg[numbytes] = '\0';

    if (strcmp(msg, "yes") != 0) {
        fprintf(stderr, "Error: Section1-3 expecting msg: %s, but received %s", "yes", msg);
        exit(1);
    } else
        printf("A file transfer can start\n");

    //Stop here
    while (1);




    /**************************************************************************
     *                            Section 2                                   *
     **************************************************************************/




    /**************************************************************************
     *                            Section 3                                   *
     **************************************************************************/


    close(sockfd);
    return 0;
}