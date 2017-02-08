#ifndef PACKET_H
#define	PACKET_H

// Packet Format: all packets sent between the client and server must have the following structure
struct packet {             // All members of the packet should be sent as a single string, each filed separated by a colon
    unsigned int total_frag;    // total number of fragments of the file. Each packet contains one fragment
    unsigned int frag_no;       // the sequence number of the fragment starting from 1
    unsigned int size;          // size of the data range:[0, 1000]
    char* filename;             
    char filedata[1000];
};

#endif	/* PACKET_H */

