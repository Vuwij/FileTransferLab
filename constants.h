#ifndef CONSTANTS_H
#define CONSTANTS_H

#define PORT "3490" // the port users will be connecting to

// The packet structure
struct packet {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

#endif

