#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>
#include "packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT      5005
#define TIMEOUT   2000   // ms
#define MAX_RETRY 5
#define LOSS_RATE 30     // % chance of simulated packet loss

// messages to send - change these to whatever
char *messages[] = {
    "hello from vedika",
    "sequence numbers working",
    "retransmission test packet",
    "reliable udp over socket",
    "this is packet number five",
    "almost there",
    "last packet sent"
};

int main() {
    srand(time(NULL));

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    // set receive timeout
    DWORD timeout = TIMEOUT;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
              (char*)&timeout, sizeof(timeout));

    struct sockaddr_in recv_addr;
    recv_addr.sin_family      = AF_INET;
    recv_addr.sin_port        = htons(PORT);
    recv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int total   = sizeof(messages) / sizeof(messages[0]);
    int retries = 0;
    int lost    = 0;
    int delivered = 0;

    printf("Sending %d messages...\n\n", total);

    for (int seq = 0; seq < total; seq++) {
        Packet pkt;
        pkt.type     = DATA;
        pkt.seq      = seq;
        pkt.length   = strlen(messages[seq]);
        pkt.checksum = compute_checksum(messages[seq], pkt.length);
        memcpy(pkt.data, messages[seq], pkt.length);

        char buf[2048];
        int  buf_len = serialize(&pkt, buf);

        int  attempts = 0;
        int  acked    = 0;

        while (!acked && attempts < MAX_RETRY) {
            // simulate packet loss
            if ((rand() % 100) < LOSS_RATE) {
                printf("[LOSS]     seq=%d dropped (simulated)\n", seq);
                lost++;
            } else {
                sendto(sock, buf, buf_len, 0,
                      (struct sockaddr*)&recv_addr, sizeof(recv_addr));
                printf("[SENT]     seq=%d msg='%s' attempt=%d\n",
                       seq, messages[seq], attempts + 1);
            }

            // wait for ACK
            char ack_buf[64];
            struct sockaddr_in from;
            int from_len = sizeof(from);
            int bytes = recvfrom(sock, ack_buf, sizeof(ack_buf), 0,
                                (struct sockaddr*)&from, &from_len);

            if (bytes > 0) {
                Packet ack;
                deserialize(ack_buf, &ack);
                if (ack.type == ACK && ack.seq == seq) {
                    printf("[ACK]      seq=%d received ✓\n\n", seq);
                    acked = 1;
                    delivered++;
                }
            } else {
                printf("[TIMEOUT]  seq=%d retrying...\n", seq);
                retries++;
            }
            attempts++;
        }

        if (!acked) {
            printf("[FAILED]   seq=%d gave up\n\n", seq);
        }
    }

    printf("========== RESULTS ==========\n");
    printf("Total messages : %d\n", total);
    printf("Delivered      : %d\n", delivered);
    printf("Retries        : %d\n", retries);
    printf("Lost (sim)     : %d\n", lost);
    printf("=============================\n");

    closesocket(sock);
    WSACleanup();
    return 0;
}