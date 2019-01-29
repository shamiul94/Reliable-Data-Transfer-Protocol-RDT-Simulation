//
// Created by shamiul93 on 1/26/19.
//

#ifndef RDT_GBN_UTIL_H
#define RDT_GBN_UTIL_H

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

#define MAX_SEQ_NO 8
#define EXTRA_BUFFER_MAX_SIZE 50
#define WINDOW_MAX_SIZE 7

/* current expected seq for B */
int B_seqnum = 0;

/* defines for statistic */
int countFromLayer5_A = 0;
int countToLayer3_A = 0;
int countFromLayer3_B = 0;
int countToLayer5_B = 0;

int nDroppedMessages = 0;

float Time = 0.000;

/* defines for sender */
/* define window size */
#define N 10
int base = 0;
int nextseq = 0;
/* ring buffer for all packets in window */
int packets_base = 0;
struct pkt packets[N];

queue<msg> msgBuffer;

/***********Variables ************/


/********* FUNCTION PROTOTYPES. DEFINED IN THE LATER PART******************/
void starttimer(int AorB, float increment);

void stoptimer(int AorB);

void tolayer3(int AorB, struct pkt packet);

void tolayer5(int AorB, char datasent[20]);

void myStartTimer(int AorB, int timerNo, float increment);

void myStopTimer(int AorB);

void myResetTimers(int AorB);

void myClearTimerFlag(int AorB);


/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

void A_output(struct msg message);

void A_input(struct pkt packet);

void B_output(struct msg message);


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
    if (AorB == A) {
        if (p != nullptr) {
            printf("[%c] %s. Window[%d,%d) Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
                   base, nextseq, p->seqnum, p->acknum, p->checksum, p->payload[0]);
        } else if (m != nullptr) {
            printf("[%c] %s. Window[%d,%d) Message[data=%c..]\n", ch, msg, base, nextseq, m->data[0]);
        } else {
            printf("[%c] %s.Window[%d,%d)\n", ch, msg, base, nextseq);
        }
    } else {
        if (p != nullptr) {
            printf("[%c] %s. Expected[%d] Packet[seq=%d,ack=%d,check=%d,data=%c..]\n", ch, msg,
                   B_seqnum, p->seqnum, p->acknum, p->checksum, p->payload[0]);
        } else if (m != nullptr) {
            printf("[%c] %s. Expected[%d] Message[data=%c..]\n", ch, msg, B_seqnum, m->data[0]);
        } else {
            printf("[%c] %s.Expected[%d]\n", ch, msg, B_seqnum);
        }
    }
}


msg pop_msg() {
    msg tem = msgBuffer.front();
    msgBuffer.pop();
    return tem;
}


bool window_isEmpty() {
    return nextseq == base;
}

int window_isfull() {
    return nextseq == ((base + WINDOW_MAX_SIZE) % MAX_SEQ_NO);
}


void free_packets(int acknum) {
    int count;

    if (acknum == nextseq)
        return;

    count = (acknum - base + 1 + MAX_SEQ_NO) % MAX_SEQ_NO;
    base = (acknum + 1) % MAX_SEQ_NO;

    for (int i = 0; i < count; ++i) myStopTimer(A);

    //when the window size decrease, check the
    //extra buffer, if we need any message need to sned

    while (count > 0 && !msgBuffer.empty()) {
        msg message = pop_msg();
        A_output(message);
        count--;
    }
}

/// also forwards nextseqnum
void addPacketToWindow(pkt &packet) {
    packets[nextseq] = packet;
    nextseq = (nextseq + 1) % MAX_SEQ_NO;
}

void retransmitCurrentWindow() {
    for (int i = base; i != nextseq; i = (i + 1) % MAX_SEQ_NO) {
        tolayer3(A, packets[i]);
        myStartTimer(A, i, TIMEOUT);
        ++countToLayer3_A;
        printLog(A, const_cast<char *>(">Resend the packet again"), &packets[i], NULL);
    }
}
/* called from layer 5, passed the data to be sent to other side */

/* Every time there is a new packet come,
 * a) we append this packet at the end of the extra buffer.
 * b) Then we check the window is full or not; If the window is full, we just leave the packet in the extra buffer;
 * c) If the window is not full, we retrieve one packet at the beginning of the extra buffer, and process it.
 */

void A_output(struct msg message) {

    printLog(A, const_cast<char *>("Received a message from layer5"), NULL, &message);

    if (msgBuffer.size() >= EXTRA_BUFFER_MAX_SIZE) {
        ++nDroppedMessages;
        printLog(A, const_cast<char *>("Extra Buffer full. Dropping Message!!!"), nullptr, &message);
        return;
    }

    msgBuffer.push(message);

/// if windows is full just push then
    if (window_isfull()) {
        printLog(A, const_cast<char *>("Window is full already, save message to extra buffer"), NULL, &message);
        return;
    }
/// window not full hence queue has only one element
//    if (msgBuffer.empty()) { // Error
//        printf("No message need to process\n");
//        return;
//    }

    msgBuffer.pop();

    ++countFromLayer5_A;
    printLog(A, const_cast<char *>("Processed an message from layer5"), NULL, &message);


    pkt packet;

    strncpy(packet.payload, message.data, strlen(message.data));
    packet.seqnum = nextseq;
    packet.acknum = DEFAULT_ACK_SEQ_NUMBER;
    packet.checksum = calc_checksum(&packet);


    addPacketToWindow(packet);

    tolayer3(A, packet);
    myStartTimer(A, nextseq, TIMEOUT);
    ++countToLayer3_A;

    printLog(A, const_cast<char *>("Send packet to layer3"), &packet, &message);
}


/* need be completed only for extra credit */
void B_output(struct msg message) {
}

/* called from layer 3, when a packet arrives for layer 4 */
/* called when A's timer goes off */
void A_input(struct pkt packet) {
    printf("================================ Inside A_input===================================\n");
    printLog(A, const_cast<char *>("Receive ACK packet from layer3"), &packet, nullptr);

/* check checksum, if corrupted, do nothing */
    if (packet.checksum != calc_checksum(&packet)) {
        printLog(A, const_cast<char *>("ACK packet is corrupted"), &packet, nullptr);
        return;
    }

    /* Duplicate ACK: NACK -> resend all */
    if (packet.acknum == ((base + WINDOW_MAX_SIZE) % MAX_SEQ_NO)) {
        printLog(A, const_cast<char *>("Received duplicate ACK"), &packet, NULL);
        myResetTimers(A);
        retransmitCurrentWindow();
        return;
    }

/* go to the next seq, and stop the timer */
    free_packets(packet.acknum);

    printLog(A, const_cast<char *>("ACK packet process successfully accomplished!!"), &packet, nullptr);
    printf("================================ Outside A_input===================================\n");
}

/* called when A's timer goes off */
void A_timerinterrupt() {
    myClearTimerFlag(A);
    myResetTimers(A);

    if (window_isEmpty()) {
        perror(">> Problem: timer interrupt occurred with empty window\n");
    }

    printLog(A, const_cast<char *>("Time interrupt occur"), nullptr, nullptr);

    /* if current packets not ACKed, resend it */
    retransmitCurrentWindow();
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
//        return;
    }

/* normal package, deliver data to layer5 */
    else if (packet.seqnum == B_seqnum) {
        tolayer5(B, packet.payload);
        B_seqnum = (B_seqnum + 1) % MAX_SEQ_NO;
        ++countToLayer5_B;
        printLog(B, const_cast<char *>("Send packet to layer5"), &packet, nullptr);
    }
/* duplicate package, do not deliver data again.
   just resend the latest ACK again */
    else if (packet.seqnum < B_seqnum) {
        printLog(B, const_cast<char *>("Duplicated packet detected"), &packet, nullptr);
    }
/* disorder packet, discard and resend the latest ACK again */
    else {
        printLog(B, const_cast<char *>("Disordered packet received"), &packet, nullptr);
    }

    /* send back ACK with the last received seqnum */
    packet.acknum = (B_seqnum + MAX_SEQ_NO - 1) % MAX_SEQ_NO;    /* resend the latest ACK */
    packet.checksum = calc_checksum(&packet);
    tolayer3(B, packet);
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


bool isTimerRunning = false;

queue<pair<float, float >> queueOfTimer;// startTime,Timeout

void myStartTimer(int AorB, int timerNo, float increment) {
    if (!isTimerRunning) {
        starttimer(AorB, increment);
        isTimerRunning = true;
        return;
    }

    queueOfTimer.push(make_pair(Time, increment));
}

void myStopTimer(int AorB) {
    if (!isTimerRunning)
        return;
    stoptimer(AorB);
    isTimerRunning = false;


    if (!queueOfTimer.empty()) {
        starttimer(AorB, (queueOfTimer.front().second - (Time - queueOfTimer.front().first)));// timeout-elapsed_time
        queueOfTimer.pop();
        isTimerRunning = true;
    }
}

void myResetTimers(int AorB) {
    if (!isTimerRunning)
        return;
    stoptimer(AorB);
    isTimerRunning = false;
    while (!queueOfTimer.empty()) queueOfTimer.pop();
}

void myClearTimerFlag(int AorB) {
    isTimerRunning = false;
}

#endif //RDT_GBN_UTIL_H
