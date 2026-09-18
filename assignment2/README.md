# Assignment 2: Basic ARQ File Transfer

Selective Repeat currently demonstrates ACK handling only: every CRC-valid frame is passed to the ACK sender, with the existing ACK loss/corruption/delay simulation. It does not buffer or deliver payloads, so its output file is empty. File reconstruction and the comparison below currently apply only to Stop-and-Wait and Go-Back-N.

Two C++ programs transfer a file using Stop-and-Wait, Go-Back-N, or Selective Repeat. TCP connects the programs; the code simulates loss, corruption, and delay in both DATA and ACK messages.

## Build

Install Boost headers, then run from this directory:

```bash
make
```

## Run

Start the receiver in one terminal:

```bash
./receiver_app 8080 received_output.txt
```

In another terminal, choose one sender command:

```bash
./sender_app stopwait sample_input.txt 1 0.1
./sender_app gobackn sample_input.txt 4 0.1
./sender_app selective sample_input.txt 4 0.1
```

Restart the receiver before each transfer. The last number is the impairment probability; use 0 for no simulated errors. Random patterns vary between runs.

Full sender syntax:
```text
./sender_app <stopwait|gobackn|selective> <file> [window] [probability] [address] [port]
```

Defaults: window 4, probability 0, address 127.0.0.1, port 8080. Stop-and-Wait always uses window 1.

Check the received file:
```bash
cmp sample_input.txt received_output.txt
```
No output means the files match.

## How it works

- `Binary.cpp`: convert bytes and numbers to bits and back.
- `CRC.cpp`: generate and check CRC-32 using XOR division.
- `Frame.cpp`: combine a 15-byte header, 46-byte padded payload, and 4-byte CRC.
- `Channel.cpp`: simulate DATA loss, corruption, and delay.
- `SocketConnection.cpp`: send and receive newline-terminated strings with receive timeouts.
- `sender.cpp`: create frames, run the selected ARQ algorithm, and retry missing frames.
- `receiver.cpp`: check frames, send ACKs, and save payloads in order.

The sender sends START with the protocol settings, followed by DATA messages. The receiver sends ACK messages back. When all frames are acknowledged, the sender sends END and the receiver saves the output.

Stop-and-Wait waits after each frame. Go-Back-N uses cumulative ACKs and resends from the oldest unacknowledged frame after a timeout. The Selective Repeat sender uses individual ACKs and timers; its receiver currently only sends ACKs.

The timeout adapts to measured RTT. Retries stop after 100 attempts per frame. Inputs must be nonempty and at most 11,776 bytes (256 frames). The sender reports only frame count, attempts, retries, and timeouts.

This basic version omits automated evaluation and detailed performance reports. Frame format, CRC, and simulated channel behavior are retained; Selective Repeat reception has been reduced to ACK handling.
