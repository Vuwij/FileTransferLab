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
#include "../packet.h"
#include <time.h>
#include <assert.h>

void usage(char *program) {
    fprintf(stderr, "Usage: %s <server address> <server port number>\n", program);
    exit(1);
}

void command_line_arguments(char* serverAddress, int* port, int argc, char *argv[]) {
    // Server Address
    strncpy(serverAddress, argv[1], MAXBUFSIZE);

    // Port Number
    *port = atoi(argv[2]);
    if (!(0 <= *port && *port <= 65535)) {
        fprintf(stderr, "port = %d should be within 0-65535\n", *port);
        usage(argv[0]);
    }
}

void user_input(char* protocolType, char* fileName) {
    while (1) {
        printf("client: ");
        scanf("%s %s", protocolType, fileName);
        if (strcmp(protocolType, "ftp") != 0)
            fprintf(stderr, "Usage: ftp <file name>\n");
        else
            break;
    }
}

int open_socket(struct addrinfo **p, char *argv[]) {
    int sockfd;
    struct addrinfo hints, *servinfo;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for ((*p) = servinfo; (*p) != NULL; (*p) = (*p)->ai_next) {
        if ((sockfd = socket((*p)->ai_family, (*p)->ai_socktype,
                (*p)->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        break;
    }

    if ((*p) == NULL) {
        fprintf(stderr, "client: failed to create socket\n");
        return 2;
    }
    
    freeaddrinfo(servinfo);
    return sockfd;
}

void verify_ftp_connection(int sockfd, char* protocolType, struct addrinfo *p) {
    int numbytes;
    
    // Sending a message protocolType "ftp" to the server
    if ((numbytes = sendto(sockfd, protocolType, strlen(protocolType), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }
    
    // Receive a message from the server:
    char msg[MAXBUFSIZE];
    if ((numbytes = recvfrom(sockfd, msg, MAXDATASIZE - 1, 0,
            p->ai_addr, &p->ai_addrlen)) == -1) {
        perror("recv");
        exit(1);
    }
    
    msg[numbytes] = '\0';
    if (strcmp(msg, "yes") != 0) {
        fprintf(stderr, "Error: Section1-3 expecting msg: %s, but received \"%s\"\n", "yes", msg);
        exit(1);
    } else
        printf("A file transfer can start\n");    
}

void send_file(char* fileName, FILE *fp, int sockfd, struct addrinfo *p) {
    // Find the size of the file
    struct stat bufStat;
    fstat(fileno(fp), &bufStat);
    int totalSize = bufStat.st_size;
    int total_frag = totalSize / 1000 + 1;
    int lastFragSize = totalSize % 1000;

    // Initialize a list of packets
    struct packet *packet_list = malloc(sizeof (struct packet) * total_frag);
    int i, bytesRead;
    for (i = 0; i < total_frag; i++) {
        if (i == total_frag - 1) {
            bytesRead = (int) fread(packet_list[i].filedata, sizeof (char), lastFragSize, fp);
            assert(bytesRead == lastFragSize);
            packet_list[i].filename = fileName;
            packet_list[i].frag_no = i;
            packet_list[i].size = lastFragSize;
            packet_list[i].total_frag = total_frag;
        } else {
            bytesRead = (int) fread(packet_list[i].filedata, sizeof (char), 1000, fp);
            assert(bytesRead == 1000);
            packet_list[i].filename = fileName;
            packet_list[i].frag_no = i;
            packet_list[i].size = 1000;
            packet_list[i].total_frag = total_frag;
        }
    }

    // Concatenate necessary info+data and send it to server
    int numbytes;
    char msg[MAXBUFSIZE];
    char tcp_packet[2000];
    
    for (i = 0; i < total_frag; i++) {

        // Initialize the buffer just incase :)
        bzero(tcp_packet, 2000);

        // Add the header to the packet
        sprintf(tcp_packet, "%d:%d:%d:", packet_list[i].total_frag, packet_list[i].frag_no, packet_list[i].size);
        int digitStringSize = strlen(tcp_packet);
        int fileNameSize = strlen(packet_list[i].filename);

        // Add the filename to the packet
        memcpy(&tcp_packet[digitStringSize], packet_list[i].filename, fileNameSize);
        memcpy(&tcp_packet[digitStringSize + fileNameSize], ":", sizeof (char));

        // Add the data to the packet
        memcpy(&tcp_packet[digitStringSize + fileNameSize + 1], packet_list[i].filedata, packet_list[i].size);

        //If it's not initial packet then wait for AWK
        if (i != 0) {
            if ((numbytes = recvfrom(sockfd, msg, MAXDATASIZE - 1, 0,
                    p->ai_addr, &p->ai_addrlen)) == -1) {
                perror("recv");
                exit(1);
            }
            if (strcmp(msg, "ACK") == 0)
                ;
            else if (strcmp(msg, "NACK") == 0)
                exit(99);
        }

        if ((numbytes = sendto(sockfd, tcp_packet, 2000, 0,
                p->ai_addr, p->ai_addrlen)) == -1) {
            perror("client: sendto");
            exit(1);
        }
    }
    
    // For printing packet information
    if(PRINT_PACKETS) {
        FILE *test = fopen("testResult.txt", "wb");
        for (i = 0; i < total_frag; i++) {
            char testingBuf[2000];

            // Preparing header
            sprintf(testingBuf, "%d:%d:%d:", packet_list[i].total_frag, packet_list[i].frag_no, packet_list[i].size);

            int digitStringSize = strlen(testingBuf);
            int fileNameSize = strlen(packet_list[i].filename);

            // Copy the filename
            memcpy(&testingBuf[digitStringSize], packet_list[i].filename, fileNameSize);
            memcpy(&testingBuf[digitStringSize + fileNameSize], ":", sizeof (char));

            // Copy the file data
            memcpy(&testingBuf[digitStringSize + fileNameSize + 1], packet_list[i].filedata, packet_list[i].size);

            // Write the data in a file 
            fwrite(testingBuf, sizeof (char), digitStringSize + fileNameSize + packet_list[i].size + 1, test);
            fwrite("\n\n\n", sizeof (char), 3, test);
        }
        fclose(test);
    }
    
    free(packet_list);
}

int main(int argc, char *argv[]) {
    if (argc != 3) usage(argv[0]);
    
    // Parse the command line arguments
    char serverAddress[MAXBUFSIZE];
    int port;
    command_line_arguments(serverAddress, &port, argc, argv);
    
    // Ask the user to input a message follows the format: ftp <file name>
    char protocolType[MAXBUFSIZE];
    char fileName[MAXBUFSIZE];
    user_input(protocolType, fileName);
    
    // Open and verify the file
    FILE *fp = fopen(fileName, "rb");
    if (!fp) {
        fprintf(stderr, "Error: the file does not exist in the folder\n");
        exit(1);
    }
    
    // Make a network connection and find the correct socket
    int sockfd;
    struct addrinfo *p;
    sockfd = open_socket(&p, argv);
    
    // Based on the above client and server, verify round trip time
    clock_t startTime, endTime, diff;
    startTime = clock();
    verify_ftp_connection(sockfd, protocolType, p);
    endTime = clock();
    diff = endTime - startTime;
    int usec = diff * 1000 * 1000 / CLOCKS_PER_SEC;
    printf("Round trip time from the client to the server %d microseconds\n", usec);
    
    // Section 3: Transfer file using packets and UDP
    send_file(fileName, fp, sockfd, p);
    
    fclose(fp);
    close(sockfd);
    return 0;
}