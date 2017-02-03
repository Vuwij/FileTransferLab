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
#include <time.h>
#include <assert.h>


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

    //Section 2
    clock_t startTime, endTime, diff;

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

    //    strcpy(protocolType, "ftp");
    //    strcpy(fileName, "1.txt");


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



    startTime = clock();

    // Sending a message protocolType "ftp" to the server
    if ((numbytes = sendto(sockfd, protocolType, strlen(protocolType), 0,
            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }


    //freeaddrinfo(servinfo);

    // 3. Receive a message from the server:
    //      a) If the message is "yes", print out "A file transfer can start"
    //      b) else, exit
    char msg[MAXBUFSIZE];

    if ((numbytes = recvfrom(sockfd, msg, MAXDATASIZE - 1, 0,
            p->ai_addr, &p->ai_addrlen)) == -1) {
        perror("recv");
        exit(1);
    }
    endTime = clock();

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

    diff = endTime - startTime;
    int usec = diff * 1000 * 1000 / CLOCKS_PER_SEC;
    printf("Round trip time from the client to the server %d microseconds\n", usec);


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
    int totalFrag = totalSize / 1000 + 1;
    int lastFragSize = totalSize % 1000;

    // Initialize packet    
    struct packet *pac = malloc(sizeof (struct packet) * totalFrag);
    int i, bytesRead;
    for (i = 0; i < totalFrag; i++) {
        if (i == totalFrag - 1) {
            bytesRead = (int) fread(pac[i].filedata, sizeof (char), lastFragSize, fp);
            assert(bytesRead == lastFragSize);
            pac[i].filename = fileName;
            pac[i].frag_no = i;
            pac[i].size = lastFragSize;
            pac[i].total_frag = totalFrag;
        } else {
            bytesRead = (int) fread(pac[i].filedata, sizeof (char), 1000, fp);
            assert(bytesRead == 1000);
            pac[i].filename = fileName;
            pac[i].frag_no = i;
            pac[i].size = 1000;
            pac[i].total_frag = totalFrag;
        }
    }

    //Testing, output goes to testResult
    FILE *test = fopen("testResult.txt", "wb");
    for (i = 0; i < totalFrag; i++) {
        char testingBuf[2000];

        //Preparing header
        sprintf(testingBuf, "%d:%d:%d:", pac[i].total_frag, pac[i].frag_no, pac[i].size);

        int digitStringSize = strlen(testingBuf);
        int fileNameSize = strlen(pac[i].filename);

        //Copy the filename
        memcpy(&testingBuf[digitStringSize], pac[i].filename, fileNameSize);
        memcpy(&testingBuf[digitStringSize + fileNameSize], ":", sizeof (char));

        //Copy the file data
        memcpy(&testingBuf[digitStringSize + fileNameSize + 1], pac[i].filedata, pac[i].size);

        //Write the data in a file 
        fwrite(testingBuf, sizeof (char), digitStringSize + fileNameSize + pac[i].size + 1, test);
        fwrite("\n\n\n", sizeof (char), 3, test);
    }

    //Concatenate necessary info+data and send it to server
    int headerSize;
    char finalPacket[2000];
    for (i = 0; i < totalFrag; i++) {

        //Initialize the buffer just incase :)
        bzero(finalPacket, 2000);

        //Start from header
        sprintf(finalPacket, "%d:%d:%d:", pac[i].total_frag, pac[i].frag_no, pac[i].size);
        int digitStringSize = strlen(finalPacket);
        int fileNameSize = strlen(pac[i].filename);

        //Copy the filename
        memcpy(&finalPacket[digitStringSize], pac[i].filename, fileNameSize);
        memcpy(&finalPacket[digitStringSize + fileNameSize], ":", sizeof (char));

        //Copy the file data
        memcpy(&finalPacket[digitStringSize + fileNameSize + 1], pac[i].filedata, pac[i].size);

        if ((numbytes = sendto(sockfd, finalPacket, 2000, 0,
                p->ai_addr, p->ai_addrlen)) == -1) {
            perror("client: sendto");
            exit(1);
        }
    }

    free(pac);

    /* You may use a simple stop-and-wait style ACK
     * The server may use ACK and NACK packets to control data flow from the sender
     * The client should open a UDP socket to listen for acknowledgements from the server
     */

    // If a file is larger than 1000 bytes, the file needs to be fragmented into smaller packets
    // with max size 1000 before transmission


    fclose(test);
    fclose(fp);
    close(sockfd);
    return 0;
}