# Reliable UDP — Custom Protocol in C

So this started as me trying to understand why TCP is designed the way it is.
UDP is fast but drops packets, reorders them, doesn't care. TCP fixes all that
but adds a lot of overhead. I wanted to build something in between — take UDP
and add just enough reliability on top manually.

Basically built my own mini protocol from scratch in C.

## What I implemented

- sequence numbers on every packet
- receiver sends ACK back for each packet
- if sender doesn't get ACK within 2 seconds → retransmit
- random packet loss simulation (30% drop rate) to test retransmission
- checksum to catch corrupted packets
- max 5 retries per packet before giving up

## How it works

```
Sender                          Receiver
  |                                |
  |--[seq=0, data, checksum]-----> |  
  |                                | verify checksum, send ACK
  |<---------[ACK seq=0]---------- |
  |                                |
  |--[seq=1]-----> (DROPPED)       |  packet lost in network
  |                                |
  | (timeout after 2s)             |
  |--[seq=1]-----> (DROPPED)       |  dropped again
  |                                |
  | (timeout again, retry 3)       |
  |--[seq=1, data, checksum]-----> |  finally gets through
  |<---------[ACK seq=1]---------- |
```

## Packet structure I designed

```
| type (1B) | seq (4B) | length (2B) | checksum (1B) | data (up to 1024B) |
```

Checksum is sum of all data bytes mod 256 — simple but catches most corruption.
Serialized manually using memcpy, similar to how registers are packed in RTL design.

## Files

```
reliable-udp/
├── packet.h       # packet struct, serialize, deserialize, checksum
├── sender.c       # sends packets, handles timeout + retransmission
├── receiver.c     # receives packets, verifies checksum, sends ACKs
├── results.png
└── README.md
```

## How to compile and run

Need GCC with C99 flag (old GCC versions need this explicitly):

```bash
# compile
gcc receiver.c -o receiver -lws2_32 -std=c99
gcc sender.c -o sender -lws2_32 -std=c99

# run receiver first (separate terminal)
.\receiver.exe

# then run sender
.\sender.exe
```

## Output I got

```
Sending 7 messages...

[SENT]     seq=0 msg='hello from vedika' attempt=1
[ACK]      seq=0 received
[SENT]     seq=1 msg='sequence numbers working' attempt=1
[ACK]      seq=1 received
[LOSS]     seq=2 dropped (simulated)
[TIMEOUT]  seq=2 retrying...
[SENT]     seq=2 msg='retransmission test packet' attempt=2
[ACK]      seq=2 received
[LOSS]     seq=5 dropped (simulated)
[TIMEOUT]  seq=5 retrying...
[LOSS]     seq=5 dropped (simulated)
[TIMEOUT]  seq=5 retrying...
[SENT]     seq=5 msg='almost there' attempt=3
[ACK]      seq=5 received

========== RESULTS ==========
Total messages : 7
Delivered      : 7
Retries        : 3
Lost (sim)     : 3
=============================
```

All 7 delivered despite 30% loss rate. Retransmission working correctly.

![Both terminals output](results/output.png)

## What I learned

Writing this in C made it very clear why TCP exists — all the things I had to
manually handle (ordering, ACKs, timeouts, retransmission) are exactly what
TCP does under the hood. Doing it at socket level in C gave a much better
feel for what's actually happening compared to just using TCP directly.

The checksum + serialize/deserialize logic reminded me a lot of RTL design —
manually controlling every byte, packing bits carefully. Same mindset.

## What I want to add later

- sliding window (send multiple packets without waiting for each ACK)
- actual packet corruption simulation not just loss
- compare throughput with raw UDP and TCP
- maybe port to Linux and test over actual network not just localhost
