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
#include <sys/stat.h>
#include "../constants.h"


#define MAXDATASIZE 1000 // max number of bytes we can get at once
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

    // Check number of arguments       
    if (argc != 3) usage(argv[0]);

    // Copy IP into serverAddress
    strncpy(serverAddress, argv[1], MAXBUFSIZE);

    // Copy the port number to be used
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
        printf("client: ");
        scanf("%s %s", protocolType, fileName);
        if (strcmp(protocolType, "ftp") != 0 || protocolType == NULL || fileName == NULL)
            fprintf(stderr, "Usage: ftp <file name>\n");
        else
            break;
    }

    // 2. Check the existence of the file:
    //      a) if exists, send a message "ftp" to the server
    //      b) else, exit

    // Open a file stream, in binary mode
    FILE *fp = fopen(fileName, "rb");

    // Check if it exists
    if (!fp) {
        fprintf(stderr, "Error: the file does not exist in the folder\n");
        exit(1);
    }

    // Make a network connection and find the correct socket
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to create socket\n");
        return 2;
    }


    // Sending a message protocolType "ftp" to the server
    if ((numbytes = sendto(sockfd, protocolType, strlen(protocolType), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);
    printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);


    // 3. Receive a message from the server:
    //      a) If the message is "yes", print out "A file transfer can start"
    //      b) else, exit
    char msg[MAXBUFSIZE];

    if ((numbytes = recv(sockfd, msg, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }
    msg[numbytes] = '\0';

    if (strcmp(msg, "yes") != 0) {
        fprintf(stderr, "Error: Section1-3 expecting msg: %s, but received \"%s\"\n", "yes", msg);
        exit(1);
    } else
        printf("A file transfer can start\n");


    /**************************************************************************
     *                            Section 2                                   *
     **************************************************************************/

    /* Based on the above client and server, you need to measure the round trip
     * time from the client to the server
     */






    /**************************************************************************
     *                            Section 3                                   *
     **************************************************************************/

    // Implement a client and a server to transfer a file
    // Unlike simply receiving a msg and sending it back,
    // You are required to have a specific packet format
    // And implement ACK for the simple file transfer using UDP socket


    // Find the size of the file
    struct stat bufStat;
    fstat(fileno(fp), &bufStat);
    int totalSize = bufStat.st_size;
    int totalFrag = totalSize / 1000;
    int lastFragSize = totalSize % 1000;

    // Initialize packet    
    struct packet pac;
    pac.filename = fileName;
    pac.frag_no = 1;
    pac.size = lastFragSize;
    pac.total_frag = totalFrag;

    // Copy the data field
    int num = fread(pac.filedata, 1, 1000, fp);

    // Initialize header of the packet
    char pacHeader[2000];
    sprintf(pacHeader, "%d:%d:%d:%s:", pac.total_frag, pac.frag_no, pac.size, pac.filename);

    //Testing
    printf("%s\n", pacHeader);

    // Sending a message protocolType "ftp" to the server
    if ((numbytes = sendto(sockfd, pacHeader, sizeof (pacHeader), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }

















    /* You may use a simple stop-and-wait style ACK
     * The server may use ACK and NACK packets to control data flow from the sender
     * The client should open a UDP socket to listen for acknowledgements from the server
     */

    // If a file is larger than 1000 bytes, the file needs to be fragmented into smaller packets
    // with max size 1000 before transmission




    close(sockfd);
    return 0;
}