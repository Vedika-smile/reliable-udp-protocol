#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <string.h>

#define DATA 0
#define ACK  1
#define MAX_DATA 1024

// packet structure - every field manually controlled
// similar to how we design registers in RTL
typedef struct {
    uint8_t  type;        // 0 = data, 1 = ack
    uint32_t seq;         // sequence number
    uint16_t length;      // payload length
    uint8_t  checksum;    // simple sum checksum
    char     data[MAX_DATA];
} Packet;

// serialize packet into raw bytes for sending
int serialize(Packet *pkt, char *buf) {
    int offset = 0;
    memcpy(buf + offset, &pkt->type,     sizeof(pkt->type));     offset += sizeof(pkt->type);
    memcpy(buf + offset, &pkt->seq,      sizeof(pkt->seq));      offset += sizeof(pkt->seq);
    memcpy(buf + offset, &pkt->length,   sizeof(pkt->length));   offset += sizeof(pkt->length);
    memcpy(buf + offset, &pkt->checksum, sizeof(pkt->checksum)); offset += sizeof(pkt->checksum);
    memcpy(buf + offset, pkt->data,      pkt->length);           offset += pkt->length;
    return offset;
}

// deserialize raw bytes back into packet
void deserialize(char *buf, Packet *pkt) {
    int offset = 0;
    memcpy(&pkt->type,     buf + offset, sizeof(pkt->type));     offset += sizeof(pkt->type);
    memcpy(&pkt->seq,      buf + offset, sizeof(pkt->seq));      offset += sizeof(pkt->seq);
    memcpy(&pkt->length,   buf + offset, sizeof(pkt->length));   offset += sizeof(pkt->length);
    memcpy(&pkt->checksum, buf + offset, sizeof(pkt->checksum)); offset += sizeof(pkt->checksum);
    memcpy(pkt->data,      buf + offset, pkt->length);
}

// basic checksum - sum of all data bytes mod 256
uint8_t compute_checksum(char *data, int len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) sum += data[i];
    return sum;
}

#endif