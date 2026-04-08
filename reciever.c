#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include "packet.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 5005

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr, sender_addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(sock, (struct sockaddr*)&addr, sizeof(addr));
    printf("Receiver listening on port %d...\n\n", PORT);

    int expected_seq = 0;
    int sender_len   = sizeof(sender_addr);
    char buf[2048];

    while (1) {
        int bytes = recvfrom(sock, buf, sizeof(buf), 0,
                            (struct sockaddr*)&sender_addr, &sender_len);
        if (bytes <= 0) break;

        Packet pkt;
        deserialize(buf, &pkt);

        if (pkt.type == DATA) {
            uint8_t chk = compute_checksum(pkt.data, pkt.length);

            if (chk != pkt.checksum) {
                printf("[CORRUPT]  seq=%d checksum mismatch!\n", pkt.seq);
            } else if (pkt.seq == expected_seq) {
                printf("[RECV]     seq=%d data='%.*s' checksum=OK\n",
                       pkt.seq, pkt.length, pkt.data);
                expected_seq++;
            } else {
                printf("[OOO]      seq=%d expected=%d out of order!\n",
                       pkt.seq, expected_seq);
            }

            // send ACK back regardless
            Packet ack;
            ack.type   = ACK;
            ack.seq    = pkt.seq;
            ack.length = 0;

            char ack_buf[64];
            int  ack_len = serialize(&ack, ack_buf);
            sendto(sock, ack_buf, ack_len, 0,
                  (struct sockaddr*)&sender_addr, sender_len);
            printf("[ACK]      seq=%d sent\n\n", pkt.seq);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}