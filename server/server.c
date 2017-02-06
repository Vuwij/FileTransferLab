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
#include <assert.h>

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

    /**************************************************************************
     *                     Parsing and Error Checking                         *
     **************************************************************************/
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

    //Insert Null at the end of the buf string
    buf[numbytes] = '\0';

    //Confirm if connection is FTP
    char *reply = (strcmp(buf, "ftp") == 0) ? "yes" : "no";

    //Send reply msg
    if ((numbytes = sendto(sockfd, reply, strlen(reply), 0,
            (struct sockaddr *) &their_addr, addr_len)) == -1) {
        perror("sendto");
        exit(1);
    }

    /**************************************************************************
     *                            Section 2                                   *
     **************************************************************************/
    //N/A

    /**************************************************************************
     *                            Section 3                                   *
     **************************************************************************/
    // Upon receiving the first packet in a sequence (frag_no = 1)
    // The program should read the file name from the packet and create a corresponding
    // file stream on the local file system
    // Data read from packets should then be written to this file stream
    // If the EOF packet is received, the file stream should be closed

    //Variables to be used
    int i;
    char receivedPacket[2000];
    char filename[1000];
    FILE * fp = NULL;
    struct packet pac;

    //Testing purpose
    int pacCount = 0; //With initial packet saved
    while (1) {

        //Empty buffer
        bzero(receivedPacket, 2000);

        //Receiving initialPacket
        if ((numbytes = recvfrom(sockfd, receivedPacket, sizeof (receivedPacket), 0,
                (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        //Parse the msg
        int j = 0;
        int colonLocation[4];
        for (i = 0; i < 2000; i++) {
            if (receivedPacket[i] == ':')
                colonLocation[j++] = i;
            if (j == 4)
                break;
        }

        //Parse integers
        pac.total_frag = atoi(receivedPacket);
        pac.frag_no = atoi(&receivedPacket[colonLocation[0] + 1]);
        pac.size = atoi(&receivedPacket[colonLocation[1] + 1]);

        //Parse filename
        bzero(filename, 1000);
        memcpy(filename, &receivedPacket[colonLocation[2] + 1], colonLocation[3] - colonLocation[2]);
        filename[colonLocation[3] - colonLocation[2] - 1] = '\x00'; //Add NULL at the end
        pac.filename = filename;

        //Save the initial packet in the array
        memcpy(pac.filedata, &receivedPacket[colonLocation[3] + 1], pac.size);

        //If it's first packet, open file stream
        if (pac.frag_no == 0) {
            assert(fp == NULL);
            fp = fopen(filename, "w+");
            assert(fp != NULL);
        }

        //Testing
        printf("%d\n", pacCount);
        pacCount++;

        //Write the data onto file stream
        fwrite(pac.filedata, sizeof (char), pac.size, fp);

        //Send AWK
        if ((numbytes = sendto(sockfd, "ACK", strlen("ACK"), 0,
                (struct sockaddr *) &their_addr, addr_len)) == -1) {
            perror("sendto");
            exit(1);
        }

        //If it was the last packet close the file stream
        if (pac.frag_no == pac.total_frag - 1) {
            int f = fclose(fp);
            assert(f == 0);
            pacCount = 0;
            fp = NULL;
            assert(fp == NULL);
            continue;
        }

    }

    close(sockfd);
    return 0;
}
