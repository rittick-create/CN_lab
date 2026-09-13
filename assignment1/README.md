
Sender side + extra things

The sender receives the input filename from the command line.

The file is converted into bits and divided into 48-byte payloads.

We will be using Ethernet IEEE 802.3 standard frame

| Preamble | SFD | Destination MAC | Source MAC | Length/Type | Data + Pad | FCS |
|  7 bytes | 1 B |      6 B        |    6 B     |     2 B     | 46–1500 B  | 4 B |

<.                      Header                                >< payload>< Trailer >


Then checksum or CRC bits are generated from each payload and added as its trailer.

After that, the selected random error-injection technique is applied.

The complete frame is then sent to the receiver.

------------------------------------------------------------------------------------------------------------

Receiver side (mainly error detection)
Receiver will check if there is any error detected. Based on the detection it will accept
or reject the received codeword.

------------------------------------------------------------------------------------------------------------


Error Injections used:- 
single-bit error, two isolated single-bit errors, odd number of errors, and burst
errors.

------------------------------------------------------------------------------------------------------------

Error detection techniques used (also used for adding redundant bits) are:-
Checksum (16-bit)
(CRC-8, CRC-10, CRC-16 and CRC-32)

------------------------------------------------------------------------------------------------------------



=> In our case, the frame header contains the source MAC, destination MAC, payload length, and error-detection type.

-> Checksum redundant bits and detection added.


------------------------------------------------------------------------------------------------------------

# This is like a simulation at transport layer using boost asio lib for socket implementation


# Reading the input file

The sender receives the input filename through the command line:

```
./sender_app textfile.txt
```


`getline()` removes the newline character, so the program manually adds `\n` back to every stored line. This ensures the newline is also converted into bits. The binary value of `\n` is:

```text
00001010
```

## Converting the file contents into bits

`CommonFunctions/StringtoBinary.cpp` contains `strToBinary()`. Every character is converted into its 8-bit representation.

For example:

```text
A -> 01000001
```

The converted file contents are written as one continuous bit string to:

```text
contentBIts.txt
```

Only the characters `0` and `1` are stored in this file.

## Frame classes

`Sender/Frame.cpp` contains four classes:

- `Header`
- `Payload`
- `Trailer`
- `Frame`

The `Frame` class contains one object of each of the other three classes:

```text
Frame = Header + Payload + Trailer
```

### Header

The header is always 128 bits and contains:

| Field | Size | Current value or purpose |
|---|---:|---|
| Source MAC | 48 bits | Temporary constant `123456789012` |
| Destination MAC | 48 bits | Temporary constant `210987654321` |
| Payload length | 16 bits | Stores `48`, meaning 48 bytes |
| Error-detection type | 16 bits | Identifies checksum or CRC type |

The MAC values are placeholders. Proper MAC-address generation can be added later.

The error-detection codes are:

| Code | Method |
|---:|---|
| 1 | Checksum-16 |
| 2 | CRC-8 |
| 3 | CRC-10 |
| 4 | CRC-16 |
| 5 | CRC-32 |

One error-detection method is selected for the complete input file. The same method code is stored in every frame header.

### Payload

Each frame carries exactly 48 bytes of payload:

```text
48 bytes x 8 = 384 bits
```

The bit stream from `contentBIts.txt` is divided into 384-bit pieces. Each piece becomes the payload of one `Frame` object, and all frames are stored in a `vector<Frame>`.

If fewer than 384 bits remain for the last frame, zeros are added at the end until the payload reaches 384 bits.

Example:

```text
Remaining data: 168 bits
Required padding: 384 - 168 = 216 zero bits
Frames created: 1
```

### Trailer

The trailer stores the redundant error-detection bits. Its length depends on the selected method:

| Method | Trailer size |
|---|---:|
| Checksum-16 | 16 bits |
| CRC-8 | 8 bits |
| CRC-10 | 10 bits |
| CRC-16 | 16 bits |
| CRC-32 | 32 bits |

The complete frame transmitted through the socket is:

```text
128-bit Header + 384-bit Payload + variable-length Trailer
```

For error detection, this project uses:

```text
Dataword = Payload
Codeword = Payload + Trailer
Complete frame = Header + Codeword
```

The header is transmitted with the codeword, but it is not included in checksum or CRC generation.

##  Selecting the error-detection method

The sender displays this menu:

```text
Select an error-detection method:
1. Checksum-16
2. CRC-8
3. CRC-10
4. CRC-16
5. CRC-32
```

The selected method is applied independently to every frame, but the same method is used for all frames belonging to one file.

## Checksum-16 generation

`ErrorDetection/Checksum/Checksum.cpp` generates the checksum for each frame.

The steps are:

1. Finish the 384-bit payload by adding padding if necessary.
2. Use the payload as the dataword.
3. Divide the payload into 16-bit words.
4. Add all 16-bit words.
5. If a carry goes beyond 16 bits, wrap it around and add it to the right side.
6. Invert every bit of the final 16-bit sum.
7. Store the resulting 16 bits in the frame trailer.

`ErrorDetection/Checksum/ChecksumDetection.cpp` performs receiver-side checking. It adds the payload and received checksum. A correct codeword produces sixteen `1` bits after one's-complement addition.

The detection function returns:

```text
true  -> checksum is valid
false -> an error is detected
```

##  CRC generation and detection

`ErrorDetection/CRC/CRC.cpp` contains one generic CRC generator. The same modulo-2 division algorithm is used for every CRC type; only the generator polynomial changes.

| Type | Generator bits | Remainder size |
|---|---|---:|
| CRC-8 | `100000111` | 8 bits |
| CRC-10 | `11000110011` | 10 bits |
| CRC-16 | `11000000000000101` | 16 bits |
| CRC-32 | `100000100110000010001110110110111` | 32 bits |

CRC generation works as follows:

1. Use the 384-bit payload as the dataword.
2. Let the CRC size be `N` bits.
3. Append `N` zeros to the payload.
4. Divide by the selected generator using XOR instead of normal subtraction.
5. Take the final `N`-bit remainder.
6. Store that remainder in the trailer.

`ErrorDetection/CRC/CRCDetection.cpp` checks a received CRC frame:

1. Combine the received payload and CRC trailer.
2. Divide the received codeword by the same generator.
3. Check the remainder.

The function returns `true` only when every remainder bit is zero.

The current CRC implementation uses the basic classroom modulo-2 division convention. It does not yet implement additional Ethernet CRC-32 settings such as reflected input/output, a non-zero initial register, or a final XOR value.

##  Error injection

Error injection occurs only after the checksum or CRC trailer has been generated. The original `Frame` objects remain unchanged. The sender creates separate complete-frame bit strings for transmission and modifies those copies.

The sender displays:

```text
Select an error-injection method:
1. No error
2. Single-bit error
3. Two isolated single-bit errors
4. Odd number of errors
5. Burst error
```

`Sender/ErrorInjection/errorInjection.cpp` implements the methods:

| Selection | Behavior |
|---|---|
| No error | Does not change the transmitted frame |
| Single-bit error | Flips one randomly selected bit |
| Two isolated errors | Flips two bits with one unchanged bit between them |
| Odd number of errors | Flips three separated bits |
| Burst error | Flips eight consecutive bits |

The selected injection method is applied to every transmitted frame. Errors are restricted to the protected codeword region, which is the payload plus trailer. The header is not modified by the current injection functions.

##  Receiver processing

`Receiver/receiver.cpp` performs these steps for every received frame:

1. Read the first 128 bits as the header.
2. Read the next 384 bits as the payload.
3. Treat all remaining bits as the trailer.
4. Read the final 16 header bits to determine the error-detection type.
5. Call either checksum detection or generic CRC detection using only the payload and trailer.
6. Stop and return `false` if any frame fails.
7. Return `true` only when every received frame passes.

The final meanings are:

```text
true  -> no error was detected in any frame
false -> an error was detected in at least one frame
```

##  Installing Boost

The socket implementation uses Boost.Asio. On macOS with Homebrew, install Boost using:

```bash
brew install boost
```

The workspace file `.vscode/c_cpp_properties.json` adds this path for VS Code IntelliSense:

```text
/opt/homebrew/opt/boost/include
```

If VS Code still shows a red underline under `boost/asio.hpp`, run these commands from the VS Code Command Palette:

```text
C/C++: Reset IntelliSense Database
Developer: Reload Window
```

## Building the sender and receiver

Run the following commands from the `CN_Lab` folder.

Build the receiver:

```bash
g++ -std=c++17 \
  -I"$(brew --prefix boost)/include" \
  Receiver/receiver.cpp -o receiver_app -pthread
```

Build the sender:

```bash
g++ -std=c++17 \
  -I"$(brew --prefix boost)/include" \
  main.cpp -o sender_app -pthread
```

##Running the programs

Two terminals are required. The receiver must be started before the sender.

### Terminal 1: receiver

```bash
cd /Users/rittickdutta/Desktop/CN_Lab
./receiver_app
```

The receiver waits on TCP port `8080` by default:

```text
Receiver waiting on port 8080...
```

### Terminal 2: sender

```bash
cd /Users/rittickdutta/Desktop/CN_Lab
./sender_app textfile.txt
```

The sender uses `127.0.0.1` and port `8080` by default.

The complete sender command format is:

```bash
./sender_app <input_file> [receiver_address] [port]
```

Example using explicit localhost values:

```bash
./sender_app textfile.txt 127.0.0.1 8080
```

To send to another computer, replace `127.0.0.1` with the receiver computer's IP address. The selected TCP port must be allowed through the receiver computer's firewall.

The receiver handles one sender connection and then exits. Restart `receiver_app` before every new sender run.

##How the Boost.Asio socket works

The socket layer uses synchronous TCP operations. This means each networking instruction waits until its current operation is complete.

### Receiver/server side

The receiver performs the following Boost.Asio operations:

1. Creates a `boost::asio::io_context`. This object provides the operating-system networking services used by Asio.
2. Creates an TCP endpoint using port `8080`, or a port supplied on the command line.
3. Creates a TCP `acceptor`.
4. Opens the acceptor, enables address reuse, binds it to the endpoint, and starts listening.
5. Creates a TCP socket.
6. Calls `acceptor.accept(socket)` and waits for the sender to connect.

Conceptually:

```text
open -> bind -> listen -> accept
```

### Sender/client side

The sender performs these Boost.Asio operations:

1. Creates its own `boost::asio::io_context`.
2. Uses a TCP resolver to convert the receiver address and port into endpoints.
3. Creates a TCP socket.
4. Calls `boost::asio::connect()` to connect to the receiver.

Conceptually:

```text
resolve receiver address -> create socket -> connect
```

### Data sent through the socket

TCP provides a continuous stream of bytes and does not preserve application message boundaries. Therefore, this project defines a small newline-based protocol.

The sender constructs this message:

```text
<number of frames>\n
<complete frame 1 bits>\n
<complete frame 2 bits>\n
...
```

For example, when one frame is sent:

```text
1\n
<header bits><payload bits><trailer bits>\n
```

The sender transmits the message using:

```text
boost::asio::write()
```

The receiver uses:

```text
boost::asio::read_until(..., '\n')
```

First, the receiver reads the frame count. It then reads exactly that many newline-terminated frame strings.

### Receiver response

After checking every frame, the receiver sends one newline-terminated result:

```text
true\n
```

or:

```text
false\n
```

The sender waits for this response with `read_until()`, displays the result, and closes its socket when the program ends.

The complete network sequence is:

```text
Receiver binds and listens
        -> Sender connects
        -> Sender sends frame count and complete frames
        -> Receiver parses and checks every frame
        -> Receiver sends true or false
        -> Both sockets close
```

## Expected behavior

With `No error` selected:

```text
Receiver result: true (no error detected)
```

With one of the implemented errors selected:

```text
Receiver result: false (error detected)
```

The checksum and all four CRC choices were tested with all five injection selections. Clean frames returned `true`, and the implemented injected-error patterns returned `false`.



## Final Boost.Asio socket summary

The receiver is the TCP server and must start first. It binds to port `8080`, listens, and waits inside `acceptor.accept(socket)`. The sender is the TCP client. It resolves the receiver address and calls `boost::asio::connect()`.

After connecting, the sender uses `boost::asio::write()` to send:

```text
frame count -> newline -> frame 1 -> newline -> frame 2 -> newline -> ...
```

The receiver uses `boost::asio::read_until()` to separate these values at each newline. It reads the detection type from the header and checks the payload-plus-trailer codeword of every frame. Finally, it writes `true\n` or `false\n` back through the same TCP connection.

```text
receiver_app                         sender_app
     |                                   |
     | bind + listen                     |
     |<----------- TCP connect ----------|
     |<------ frame count + frames -------|
     |                                   |
     | checksum/CRC verification         |
     |                                   |
     |----------- true/false ----------->|
     |                                   |
     | close                             | close
```

Boost.Asio manages the operating-system socket operations, while the project-defined newline protocol explains where one frame ends and the next frame begins inside TCP's continuous byte stream.
