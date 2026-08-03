# Computer Networks Lab - Assignment 1

## Error Detection using Checksum and CRC

This is a simple C++ program for the Computer Networks Lab assignment. It follows this basic flow:

```text
Text file -> String -> Bits -> Frames -> Checksum/CRC -> Error -> Receiver
```

The code is written in a beginner-friendly style. It uses simple classes, vectors, loops, `if` statements, and separate `.cpp` files. There are no header files, test folders, Python files, build folders, or external libraries.

## What is completed

- Read the test filename from the command line.
- Convert every character of the file into 8 binary bits.
- Divide the bits into frames.
- Use a 46-byte or 368-bit payload, which is the minimum Ethernet payload.
- Pad the last payload with zeroes when needed.
- Generate and check a 16-bit one's-complement checksum.
- Generate and check CRC-8, CRC-10, CRC-16, and CRC-32.
- Inject single-bit, two isolated bit, odd-number, and burst errors.
- Accept a correct codeword and reject a detected error.
- Compare detection percentage, execution time, and extra check bits.
- Show a simple ASCII graph.
- Show cases where both schemes detect an error and where only one scheme detects it.

## Folder structure

```text
CN_Lab/
|-- main.cpp
|-- textfile.txt
|-- README.md
|-- CommonFunctions/
|   `-- binaryToString.cpp
|-- Sender/
|   |-- Frame.cpp
|   |-- ErrorInjection/
|   |   `-- errorInjection.cpp
|   |-- Input file(Bits)/
|   |   `-- input_bits.txt
|   `-- Frames/
|       |-- clean_frame_1.txt
|       `-- sent_frame_1.txt
|-- ErrorDetection/
|   |-- Checksum/
|   |   `-- Checksum.cpp
|   `-- CRC/
|       `-- CRC.cpp
|-- Receiver/
|   |-- receiver.cpp
|   `-- result.txt
|-- Evaluation/
|   `-- evaluation.cpp
`-- Results/
    `-- evaluation.txt
```

The output files are overwritten when the program is run again.

## Source-code size

Every source file is below 200 lines. All `.cpp` files together contain 792 lines.

| File | Lines |
|---|---:|
| `main.cpp` | 171 |
| `Evaluation/evaluation.cpp` | 174 |
| `ErrorDetection/CRC/CRC.cpp` | 133 |
| `Sender/ErrorInjection/errorInjection.cpp` | 91 |
| `ErrorDetection/Checksum/Checksum.cpp` | 78 |
| `CommonFunctions/binaryToString.cpp` | 64 |
| `Sender/Frame.cpp` | 49 |
| `Receiver/receiver.cpp` | 32 |
| Total | 792 |

## Requirement

Install a C++ compiler that supports C++17. Check it with:

```bash
g++ --version
```

## Step 1 - Compile

Open the terminal inside the `CN_Lab` folder and run:

```bash
g++ -std=c++17 main.cpp -o cn_lab
```

Only `main.cpp` is compiled because it includes the other `.cpp` files. This keeps the compile command easy for a college lab demonstration.

### Compile example

```bash
cd /Users/rittickdutta/Desktop/CN_Lab
g++ -std=c++17 main.cpp -o cn_lab
```

If there is no compiler error, the program is ready.

## Step 2 - Run

Pass the input filename on the command line:

```bash
./cn_lab textfile.txt
```

The program displays this menu:

```text
1. Checksum-16
2. CRC
3. Evaluate all schemes
```

## Step 3 - String to binary conversion

`CommonFunctions/binaryToString.cpp` reads the complete file and converts each character into 8 bits from the most significant bit to the least significant bit.

For example, capital `A` has ASCII value 65:

```text
A -> 01000001
```

The current `textfile.txt` contains:

```text
This is Saikat Nayak
```

It has 20 characters, so it becomes:

```text
20 x 8 = 160 bits
```

### Conversion example

Run the program once and open:

```text
Sender/Input file(Bits)/input_bits.txt
```

The file contains the converted input bits in groups of 8.

## Step 4 - Frame creation

`Sender/Frame.cpp` divides the input into payloads of 368 bits:

```text
46 bytes x 8 = 368 bits
```

If the final frame has fewer than 368 bits, zeroes are added at the end.

### Framing example

For the 160-bit sample input:

```text
Original bits = 160
Zero padding  = 368 - 160 = 208
Total frames  = 1
```

For a larger file, more frames are created automatically.

## Step 5 - Checksum-16

The checksum code is in `ErrorDetection/Checksum/Checksum.cpp`.

Simple working steps:

1. Divide the 368 payload bits into 16-bit words.
2. Add all words.
3. Add the carry back to the sum.
4. Complement every bit of the final 16-bit sum.
5. Add the 16 checksum bits to the payload.
6. The receiver adds everything again.
7. A final value of all `1`s means that no error was found.

### Checksum example without an error

Run:

```bash
./cn_lab textfile.txt
```

Enter:

```text
1
0
```

Example result:

```text
Check bits       : 1010110100100110
Changed positions: None
Receiver result  : ACCEPTED - No checksum error
```

### Checksum example with a single-bit error

Run the same command and enter:

```text
1
1
```

Example result:

```text
Receiver result: REJECTED - Checksum detected an error
```

The random changed position can be different on every run.

## Step 6 - CRC

CRC code is in `ErrorDetection/CRC/CRC.cpp`. It uses modulo-2 division, which means XOR is used instead of normal subtraction.

Simple working steps:

1. Select a generator polynomial.
2. Add zero bits equal to the degree of the polynomial.
3. Divide using XOR.
4. Take the remainder as the CRC bits.
5. Attach the remainder to the payload.
6. The receiver divides the received codeword again.
7. An all-zero remainder means that no error was found.

### CRC polynomials from the assignment

| Choice | CRC | Polynomial | Binary divisor | Extra bits |
|---:|---|---|---|---:|
| 1 | CRC-8 | x^8 + x^7 + x^6 + x^4 + x^2 + 1 | `111010101` | 8 |
| 2 | CRC-10 | x^10 + x^9 + x^5 + x^4 + x + 1 | `11000110011` | 10 |
| 3 | CRC-16 | x^16 + x^15 + x^2 + 1 | `11000000000000101` | 16 |
| 4 | CRC-32 | x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1 | `100000100110000010001110110110111` | 32 |

### CRC-8 example

Run the program and enter:

```text
2
1
1
```

This means CRC, CRC-8, and a single-bit error.

```text
Check bits      : 01010101
Receiver result : REJECTED - CRC detected an error
```

### CRC-10 example

Enter:

```text
2
2
2
```

This means CRC-10 with two isolated bit errors.

```text
Check bits      : 1010000101
Receiver result : REJECTED - CRC detected an error
```

### CRC-16 example

Enter:

```text
2
3
3
```

This means CRC-16 with three errors, which is an odd number.

```text
Check bits      : 0100000100101000
Receiver result : REJECTED - CRC detected an error
```

### CRC-32 example

Enter:

```text
2
4
4
```

This means CRC-32 with a four-bit burst error.

```text
Check bits      : 10101101110110101000010010011101
Receiver result : REJECTED - CRC detected an error
```

## Step 7 - Error injection

The error code is in `Sender/ErrorInjection/errorInjection.cpp`. Errors are added after the clean checksum or CRC codeword is created.

| Choice | Error | Simple action |
|---:|---|---|
| 0 | No error | Do not change any bit. |
| 1 | Single-bit | Flip one random payload bit. |
| 2 | Two isolated bits | Flip two different, non-adjacent payload bits. |
| 3 | Odd number | Flip three different payload bits. |
| 4 | Burst | Flip four consecutive payload bits. |

### Error-injection example

A burst run may show:

```text
Changed positions: 225, 226, 227, 228
```

The position changes because `rand()` is used.

## Step 8 - Receiver

`Receiver/receiver.cpp` calls the correct checking method.

- If an error is detected, the frame is rejected.
- If no error is detected, the frame is accepted.

### Receiver examples

```text
No injected error -> ACCEPTED - No checksum error
Injected error    -> REJECTED - CRC detected an error
```

The full result of the latest normal run is saved in:

```text
Receiver/result.txt
```

## Step 9 - Evaluation, table, graph, time, and resource use

Run:

```bash
./cn_lab textfile.txt
```

Enter:

```text
3
```

The evaluator uses the first frame and performs 100 trials for every scheme and every error type. It prints:

- A detection table.
- Average time in microseconds.
- An ASCII graph where one `#` represents 5% detection.
- The three comparison cases requested in the assignment.
- The number of extra check bits used by every scheme.

### Short example result

```text
Scheme       Error      Detected
Checksum-16  Single     100/100
Checksum-16  Two bits    98/100
CRC-8        Single     100/100
CRC-10       Burst      100/100
CRC-16       Odd bits   100/100
CRC-32       Burst      100/100
```

Example ASCII graph:

```text
Checksum-16  Two bits   ################### 98%
CRC-8        Single     #################### 100%
CRC-32       Burst      #################### 100%
```

Random trials and timing can change on different runs and computers.

## Special comparison cases

The evaluator uses a fixed 46-byte sample containing values `00` to `2D`. This makes the comparison repeatable.

| Case | Checksum | CRC |
|---|---|---|
| Error detected by both | Detected | Detected |
| Error detected by CRC but not checksum | Missed | Detected |
| Error detected by checksum but not CRC | Detected | Missed |

The checksum-only case uses an error pattern equal to the selected CRC generator. That pattern is divisible by the generator, so that CRC misses it. The CRC-only case uses two bit changes that cancel during checksum addition, while CRC still notices their positions.

## Resource comparison

The simple resource measurement is the number of extra bits added to one frame.

| Scheme | Extra bits | Codeword size for a 368-bit payload |
|---|---:|---:|
| CRC-8 | 8 | 376 bits |
| CRC-10 | 10 | 378 bits |
| Checksum-16 | 16 | 384 bits |
| CRC-16 | 16 | 384 bits |
| CRC-32 | 32 | 400 bits |

CRC-8 uses the least extra space. CRC-32 uses the most extra space and normally takes more calculation time, but it provides stronger error detection.

The complete generated evaluation is saved in:

```text
Results/evaluation.txt
```

## Generated files

After a normal checksum or CRC run:

| File | Meaning |
|---|---|
| `Sender/Input file(Bits)/input_bits.txt` | Original file converted to bits. |
| `Sender/Frames/clean_frame_1.txt` | Payload with correct checksum or CRC. |
| `Sender/Frames/sent_frame_1.txt` | Codeword after optional error injection. |
| `Receiver/result.txt` | Receiver decision and changed positions. |

If the input needs more than one frame, the program creates frame 2, frame 3, and so on.

## Use another input file

Create any text file inside the folder, for example `message.txt`, and run:

```bash
./cn_lab message.txt
```

An absolute or relative file path can also be passed.

## Important notes

- This is a simulation. It does not send data through real network sockets.
- It simulates the minimum 46-byte Ethernet payload, not the complete MAC header.
- Error positions are random, so normal demonstration output can change.
- The evaluation percentages are experimental results, not mathematical guarantees.
- Timing depends on the compiler and computer.
- Compile only `main.cpp`; compiling every `.cpp` separately will create duplicate definitions because `main.cpp` includes them.

## Quick demonstration order

Use this order during the lab demonstration:

1. Compile the program.
2. Show `textfile.txt`.
3. Run checksum with no error and show acceptance.
4. Run checksum with a single-bit error and show rejection.
5. Run CRC-8, CRC-10, CRC-16, and CRC-32.
6. Show all four error types.
7. Open the clean and sent frame files.
8. Open `Receiver/result.txt`.
9. Run evaluation choice 3.
10. Open `Results/evaluation.txt` and explain the table, graph, time, and resource use.
