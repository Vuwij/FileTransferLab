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
#include "../packet.h"
#include <assert.h>

void usage(char *program) {
    fprintf(stderr, "Usage: %s <UDP listen port>\n", program);
    exit(1);
}

int open_server_socket(struct addrinfo **p, char* argv[]) {
    struct addrinfo hints, *servinfo;
    int sockfd;
    int rv;
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET for IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // Using my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // looping through all results and bind the first one available
    for ((*p) = servinfo; (*p) != NULL; (*p) = (*p)->ai_next) {
        if ((sockfd = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, (*p)->ai_addr, (*p)->ai_addrlen) == -1) {
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
    
    return sockfd;
}

void accept_ftp_connection(int sockfd, struct addrinfo *p) {
    int numbytes;
    socklen_t addr_len;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFSIZE];
    
    // Receive a message called ftp
    printf("server: waiting to recvfrom...\n");
    addr_len = sizeof their_addr;

    if ((numbytes = recvfrom(sockfd, buf, MAXBUFSIZE - 1, 0,
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
}

/**
 * Section 3
 * Upon receiving the first packet in a sequence (frag_no = 1)
 * The program should read the file name from the packet and create a corresponding
 * file stream on the local file system
 * Data read from packets should then be written to this file stream
 * If the EOF packet is received, the file stream should be closed
 */
void receive_file(int sockfd, struct addrinfo *p) {
    int i;
    int numbytes;
    char receivedPacket[2000];
    char filename[1000];
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    addr_len = sizeof their_addr;
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
        if(PRINT_PACKETS)
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
}

int main(int argc, char *argv[]) {
    if (argc != 2) usage(argv[0]);

    // Obtain the Port Number
    int port = atoi(argv[1]);
    if (!(0 <= port && port <= 65535)) {
        fprintf(stderr, "port = %d should be within 0-65535\n", port);
        usage(argv[0]);
    }
    
    // Open a server side socket
    int sockfd;
    struct addrinfo *p;
    sockfd = open_server_socket(&p, argv);
    
    // Accept the ftp connection
    accept_ftp_connection(sockfd, p);
    
    // Receive a file
    receive_file(sockfd, p);

    close(sockfd);
    return 0;
}
