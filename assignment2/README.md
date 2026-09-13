a# Assignment 2 - Data Link Layer Flow Control

This project implements the three flow-control/ARQ schemes required in the assignment:

1. Stop-and-Wait ARQ
2. Go-Back-N ARQ
3. Selective Repeat ARQ

The implementation uses simple C++ classes, functions, loops, vectors, maps and conditionals at the same general syntax level as Assignment 1. Boost.Asio provides a real TCP socket connection between a separate sender program and receiver program. Packet loss, delay and corruption are simulated at the application level because TCP itself normally repairs these problems.

## Project structure

```text
assignment2/
|-- CommonFunctions/
|   |-- Binary.cpp                 byte/number and bit conversions
|   `-- SocketConnection.cpp       line messages and receive timeout
|-- ErrorDetection/
|   `-- CRC.cpp                    CRC-32 generation and checking
|-- Sender/
|   |-- Channel.cpp                delay, loss and bit-error simulation
|   |-- Frame.cpp                  header, payload, FCS and Frame class
|   `-- sender.cpp                 all three sender ARQ algorithms
|-- Receiver/
|   `-- receiver.cpp               checking, ACKs, ordering and buffering
|-- Evaluation/
|   |-- evaluate.py                automated probability comparison
|   `-- results.csv                latest measured results
|-- Makefile
|-- README.md
`-- sample_input.txt
```

## Data frame structure

Every complete data frame is represented as a string of `0` and `1` characters while it is in the simulated channel.

| Part | Field | Size |
|---|---|---:|
| Header | Source address (`SRC001`) | 6 bytes |
| Header | Destination address (`DST001`) | 6 bytes |
| Header | Actual payload length | 2 bytes |
| Header | Frame sequence number | 1 byte |
| Data | Fixed payload | 46 bytes |
| Trailer | CRC-32 frame check sequence | 4 bytes |

The final payload is padded with zero bits to 46 bytes. The 2-byte length field tells the receiver how many original bytes it should recover, so padding is not written to the output file.

The fields stated in the assignment (`6 + 6 + 2 + 1`) produce a **15-byte header**. The figure labels this as 12 bytes, but that printed total is inconsistent with its own field sizes. This implementation keeps every required field and therefore uses the correct 15-byte total.

The one-byte sequence field supports values 0 through 255. For that reason, this classroom simulation accepts a maximum of 256 frames, or 11,776 input bytes when the payload is 46 bytes.

## Framing and CRC-32

`SenderProgram::Framing()` opens the input in binary mode. This preserves every byte, including newline characters. It divides the contents into 46-byte pieces and constructs one `Frame` per piece.

The CRC module uses the same basic modulo-2 division and generator polynomial used by Assignment 1:

```text
100000100110000010001110110110111
```

For each frame:

1. The sender combines the header and padded payload.
2. It appends 32 zero bits.
3. Modulo-2 division produces a 32-bit remainder.
4. The remainder is stored as the trailer/FCS.
5. The receiver divides `header + payload + received FCS` by the same generator.
6. An all-zero remainder means the frame is accepted; any other remainder means it is discarded.

CRC protects the header as well as the data. Therefore, a corrupted sequence number or length is detected before either value is used.

## Simulated channel

`DataChannel::Channel()` adds a small normal propagation delay of 5-20 ms to every data transmission. The probability supplied on the command line is then applied independently to the DATA direction and ACK direction.

When a DATA frame is selected for impairment, the channel randomly chooses one of these cases:

- Drop the frame.
- Flip one random bit; CRC detects it at the receiver.
- Delay it beyond the current deadline and treat it as lost.

When an ACK is selected for impairment, the receiver randomly chooses one of these cases:

- Drop the ACK.
- Corrupt the ACK text so the sender ignores it.
- Delay the ACK long enough that a sender timeout can occur.

With probability `0`, none of these impairments is introduced. Random seeds make an experiment repeatable.

## Required sender methods

The assignment's sender operations appear directly in `Sender/sender.cpp`:

- `Framing()` reads the file and prepares fixed-size frames.
- `DataChannel::Channel()` simulates the transmission channel.
- `Send()` selects Stop-and-Wait, Go-Back-N or Selective Repeat.
- `Timer()` returns the current retransmission timeout.
- `Timeout()` records a recent RTT and recalculates the timeout.
- `Recv()` receives and decodes an acknowledgement.

Each transmission has a starting time. After an ACK arrives, the measured round-trip time is used to update the timeout:

```text
EstimatedRTT = 0.875 * old EstimatedRTT + 0.125 * recent RTT
Deviation    = 0.75  * old Deviation    + 0.25  * absolute RTT error
Timeout      = EstimatedRTT + 4 * Deviation
```

The timeout is limited to 100-2000 ms so that the simulation remains usable under both fast and heavily impaired conditions.

## Required receiver methods

The receiver operations appear directly in `Receiver/receiver.cpp`:

- `Recv()` is called for every incoming DATA frame.
- `Check()` validates its size and CRC-32 FCS.
- `Send()` creates an ACK and passes it through the simulated ACK channel.

Only valid data is delivered to the output. The output file is opened in binary mode and is therefore a byte-for-byte reconstruction of the sender's input.

## Stop-and-Wait ARQ

The sender window and receiver window both effectively contain one frame.

1. The sender transmits frame `i` and starts its timer.
2. The receiver checks the frame.
3. A valid expected frame is accepted and ACK `i` is returned.
4. The sender moves to `i + 1` only after ACK `i` arrives.
5. Loss, corruption or an unusable ACK causes the timer to expire.
6. After a timeout, the same frame is retransmitted.

If the receiver gets a duplicate because its original ACK was lost, it discards the duplicate data but sends the ACK again.

## Go-Back-N ARQ

The sender window is the command-line value `N`; the receiver window is 1.

1. The sender transmits up to `N` frames without waiting between individual frames.
2. The receiver accepts only the next expected sequence number.
3. Out-of-order and corrupted frames are discarded.
4. ACK `i` is cumulative: it acknowledges frames `0` through `i`.
5. A cumulative ACK moves the sender's base forward.
6. After a window timeout, the sender goes back to the oldest unacknowledged frame and retransmits from there.

The receiver repeatedly ACKs its highest in-order frame. This also lets the sender recover if a cumulative ACK is lost.

## Selective Repeat ARQ

Both sender and receiver windows have size `N`.

1. The sender transmits every new frame in its current window.
2. Each frame has an independent timer and ACK state.
3. The receiver accepts valid frames even when they arrive out of order.
4. Out-of-order payloads are held in a `map<int, string>` buffer.
5. ACK `i` acknowledges only frame `i`; it is not cumulative.
6. Only an unacknowledged frame whose own timer expires is retransmitted.
7. Buffered payloads are delivered in sequence when the missing earlier frame arrives.

This avoids resending correctly received later frames, which is the main advantage of Selective Repeat in a lossy channel.

## Build

Boost headers must be installed. On macOS with Homebrew:

```bash
brew install boost
```

Build both programs from inside `assignment2`:

```bash
make
```

The Makefile creates:

```text
sender_app
receiver_app
```

## Run one experiment

Use two terminals. Start the receiver first:

```bash
./receiver_app 8080 received_output.txt
```

Then start one sender. The sender syntax is:

```text
./sender_app <protocol> <input_file> [window] [probability] [address] [port] [seed]
```

Stop-and-Wait without errors:

```bash
./sender_app stopwait sample_input.txt 1 0.0 127.0.0.1 8080 42
```

Go-Back-N with window 4 and impairment probability 0.3:

```bash
./sender_app gobackn sample_input.txt 4 0.3 127.0.0.1 8080 42
```

Selective Repeat with the same settings:

```bash
./sender_app selective sample_input.txt 4 0.3 127.0.0.1 8080 42
```

The receiver handles one file-transfer session and then exits. Start it again before each new sender experiment.

Valid protocol spellings are:

```text
stopwait or stop-and-wait
gobackn or go-back-n
selective or selective-repeat
```

## Statistics and efficiency

After a successful transfer, the sender prints:

- Original frame count
- Total transmissions
- Retransmissions
- Timeout count
- Average measured RTT
- Final adaptive timeout
- Total transfer time
- Counts of simulated data drops, corruptions and excessive delays

Efficiency is calculated as:

```text
efficiency = original frames / total transmissions * 100 percent
```

A perfect run sends every original frame exactly once and therefore has 100% transmission efficiency. Retransmissions reduce this value.

## Automated evaluation

The supplied evaluation program runs all three schemes at probabilities:

```text
0.0, 0.1, 0.2, 0.3, 0.4 and 0.5
```

Run one trial for every protocol/probability pair:

```bash
make evaluate
```

Run several repetitions for more reliable averages:

```bash
python3 Evaluation/evaluate.py 5
```

For each case, the script:

1. Selects a free local TCP port.
2. Starts a new receiver.
3. Runs the sender with a repeatable seed.
4. Waits for retransmissions to finish.
5. Verifies that the received output is byte-for-byte identical to `sample_input.txt`.
6. Stores the measured statistics in `Evaluation/results.csv`.

The included CSV is the latest complete 18-case run. All cases from probability 0.0 through 0.5 delivered an identical output file. Individual timing results depend on the chosen random seeds and the computer running the experiment, so several repetitions should be averaged before drawing a final performance conclusion.

## Expected comparison

- With no loss or error, Stop-and-Wait is usually slowest because it waits after every frame. Go-Back-N and Selective Repeat can keep several frames in flight.
- As impairment probability rises, every approach performs more retransmissions and its efficiency falls.
- Go-Back-N is simpler at the receiver but may retransmit several already received frames after one missing frame.
- Selective Repeat requires receiver buffering and an ACK/timer for each frame, but it retransmits only missing or unacknowledged frames.
- A single short run can be noisy. The comparison is most meaningful when the same file, probabilities, window and several seeds are used for every protocol.

## Clean generated programs and results

```bash
make clean
```

This removes the compiled executables and generated evaluation output. It does not remove the source code or README.
