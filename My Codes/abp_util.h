//
// Created by shamiul93 on 1/26/19.
//


#include <bits/stdc++.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;


/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: SLIGHTLY MODIFIED
 FROM VERSION 1.1 of J.F.Kurose

   This code should be used for PA2, unidirectional or bidirectional
   data transfer protocols (from A to B. Bidirectional transfer of data
   is for extra credit and is not required).  Network properties:
   - one way network delay averages five time units (longer if there
       are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
       or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
       (although some can be lost).
**********************************************************************/

#define BIDIRECTIONAL 0 /* change to 1 if you're doing extra credit */
/* and write a routine called B_output */

/* a "msg" is the data unit passed from layer 5 (teachers code) to layer  */
/* 4 (students' code).  It contains the data (characters) to be delivered */
/* to layer 5 via the students transport level protocol entities.         */
struct msg {
    char data[20];
};

/* a packet is the data unit passed from layer 4 (students code) to layer */
/* 3 (teachers code).  Note the pre-defined packet structure, which all   */
/* students must follow. */
struct pkt {
    int seqnum;
    int acknum;
    int checksum;
    char payload[20];
};

/********* Variables **************/

/* common defines */
#define   A    0
#define   B    1
/* timeout for the timer */
#define TIMEOUT 20.0
/* when A send a data package to B,
   we do not need to set acknum,
   so we set it to this default number */
#define DEFAULT_ACK_SEQ_NUMBER 111

/* define of stages for sender */
enum sender_flag {
    WAIT_FOR_PKT,
    WAIT_FOR_ACK
};

/* current expected seq for A */
int A_seqnum = 0;
/* current expected seq for B */
int B_seqnum = 0;

/* defines for statistic */
int countFromLayer5_A = 0;
int countToLayer3_A = 0;
int countFromLayer3_B = 0;
int countToLayer5_B = 0;

float Time = 0.000;

/* store current flag for retransmit. */
struct pkt cur_packet;
/* current stage of A */
enum sender_flag A_flag = WAIT_FOR_PKT;

/***********Variables ************/


/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);

void stoptimer(int AorB);

void tolayer3(int AorB, struct pkt packet);

void tolayer5(int AorB, char datasent[20]);


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

void A_output(struct msg message);

void A_input(struct pkt packet);


/* called from layer 5, passed the data to be sent to other side */

int calc_checksum(struct pkt *p) {
    int i;
    int checksum = 0; /*First init to Zero*/
    if (p == nullptr) {
        return checksum;
    }
/*Add all the characters in payload*/
    for (i = 0; i < 20; i++) {
        checksum += (unsigned char) p->payload[i];
    }
    /*add the seqnum*/
    checksum += p->seqnum;
    /*add the acknum*/
    checksum += p->acknum;
/*Then we get the final checksum.*/
    return checksum;
}

void printLog(int AorB, char *msg, struct pkt *p, struct msg *m) {
    char ch = (AorB == A) ? 'A' : 'B';
    if (p != nullptr) {
        printf("[%c] %s. Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch,
               msg, p->seqnum, p->acknum, p->checksum, p->payload[0]);
    } else if (m != nullptr) {
        printf("[%c] %s. Message[data=%c..]\n", ch, msg, m->data[0]);
    } else {
        printf("[%c] %s.\n", ch, msg);
    }
}


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
    printf("================================ Inside A_output===================================\n");
    int i;
    int checksum = 0;

    ++countFromLayer5_A;
    printLog(A, const_cast<char *>("Receive an message from layer5"), nullptr, &message);

/* If the last packet have not been ACKed, just drop this message */
    if (A_flag != WAIT_FOR_PKT) {
        printLog(A, const_cast<char *>("Drop this message, last message have not finished"), nullptr, &message);
        return;
    }
/* set current package to not finished */
    A_flag = WAIT_FOR_ACK;

/* copy data from meg to pkt */
    for (i = 0; i < 20; i++) {
        cur_packet.payload[i] = message.data[i];
    }
/* set current seqnum */
    cur_packet.seqnum = A_seqnum;
/* we are send package, do not need to set acknum */
    cur_packet.acknum = DEFAULT_ACK_SEQ_NUMBER;

/* calculate check sum including seqnum and acknum */
    checksum = calc_checksum(&cur_packet);
/* set check sum */
    cur_packet.checksum = checksum;

/* send pkg to layer 3 */
    tolayer3(A, cur_packet);
    ++countToLayer3_A;
    printLog(A, const_cast<char *>("Send packet to layer3"), &cur_packet, &message);

/* start timer */
    starttimer(A, TIMEOUT);


    printf("================================ Outside A_output===================================\n");
}

/* need be completed only for extra credit */
void B_output(struct msg message) {
}


/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
    printf("================================ Inside A_input===================================\n");
    printLog(A, const_cast<char *>("Receive ACK packet from layer3"), &packet, nullptr);

/* check checksum, if corrupted, do nothing */
    if (packet.checksum !=
        calc_checksum(&packet)
            ) {
        printLog(A, const_cast<char *>("ACK packet is corrupted"), &packet, nullptr);
        return;
    }

/* Delay ack? if ack is not expected, do nothing */
    if (packet.acknum != A_seqnum) {
        printLog(A, const_cast<char *>("ACK is not expected"), &packet, nullptr);
        return;
    }

/* go to the next seq, and stop the timer */
    A_seqnum = (A_seqnum + 1) % 2;
    stoptimer(A);
/* set the last package have received correctly */
    A_flag = WAIT_FOR_PKT;

    printLog(A, const_cast<char *>("ACK packet process successfully accomplished!!"), &packet, nullptr);

    printf("================================ Outside A_input===================================\n");
}
/* called when A's timer goes off */
void A_timerinterrupt() {
    printLog(A, const_cast<char *>("Time interrupt occur"), &cur_packet, nullptr);
    /* if current package not finished, we resend it */
    if (A_flag == WAIT_FOR_ACK) {
        printLog(A, const_cast<char *>("Timeout! Send out the package again"), &cur_packet, nullptr);
        tolayer3(A, cur_packet);
        ++countToLayer3_A;
        /* start the timer again */
        starttimer(A, TIMEOUT);
    } else {
        printLog(A, const_cast<char *>("BUG??? Maybe forgot to stop the timer"), &cur_packet, nullptr);
    }
}
/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
}


/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
    printf("================================ Inside B_input===================================\n");
    printLog(B, const_cast<char *>("Receive a packet from layer3"), &packet, nullptr);
    ++countFromLayer3_B;

/* check checksum, if corrupted, just drop the package */
    if (packet.checksum != calc_checksum(&packet)) {
        printLog(B, const_cast<char *>("Packet is corrupted"), &packet, nullptr);
        return;
    }

/* duplicate package, do not deliver data again.
   just resend the ACK again */
    if (packet.seqnum != B_seqnum) {
        printLog(B, const_cast<char *>("Duplicated packet detected"), &packet, nullptr);
    }
/* normal package, deliver data to layer5 */
    else {
        B_seqnum = (B_seqnum + 1) % 2;
        tolayer5(B, packet.payload);
        ++countToLayer5_B;
        printLog(B, const_cast<char *>("Send packet to layer5"), &packet, nullptr);
    }

/* send back ack */
    packet.acknum = packet.seqnum;
    packet.checksum = calc_checksum(&packet);

    tolayer3(B, packet);
    printLog(B, const_cast<char *>("Send ACK packet to layer3"), &packet, nullptr);
    printf("================================ Outside B_input(packet) =========================\n");
}

/* called when B's timer goes off */
void B_timerinterrupt() {
    printf("  B_timerinterrupt: B doesn't have a timer. ignore.\n");
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {

}

