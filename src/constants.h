#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PRINT_PACKETS       1       // For testing file

#define MAXBUFSIZE          100     // For string length
#define MAXDATASIZE         1000    // max number of bytes we can get at once

// Server Parameters
#define BACKLOG             10      // how many pending connections queue will hold

#define PACKET_TIMEOUT      5000    // Timeout for packet sending (in microseconds)
#define PACKET_TIMEOUT_S    100
#define SEND_NACK           1

#define DEBUG               1       // For printing values


#endif

