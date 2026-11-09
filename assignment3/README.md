# Assignment 3: Basic CSMA in C++

A small sender/receiver simulation of Non-Persistent, 1-Persistent, p-Persistent, and CSMA/CD. Each station runs in an independent process and communicates with the shared channel through TCP. C++17 on macOS or Linux is required; no extra libraries or Python are needed.

## Build and run

From this directory:

```bash
make
./evaluate_app
./evaluate_app 8 non
./evaluate_app 8 one
./evaluate_app 8 p 5 0.25
./evaluate_app 8 cd
```

The evaluator starts station processes with `fork`; its parent acts as the receiver. Full syntax:

```text
./evaluate_app [stations scheme [frames=5] [p=0.5] [seed=1]]
```

For a manual two-station run, start the receiver:

```bash
./receiver_app 2 cd
```

Start these senders in two other terminals:

```bash
./sender_app 0 cd
./sender_app 1 cd
```

Manual syntax:

```text
./receiver_app stations scheme [frames=5] [port=8080] [transmission=4] [propagation=1]
./sender_app station scheme [frames=5] [p=0.5] [port=8080] [seed=1] [destination=other station for 0/1] [message]
```

The short sender commands automatically send "Hello from station 0" to station 1 and "Hello from station 1" to station 0, with five frames each. Stations numbered 2 or higher default to channel-only delivery.

All stations must have unique numbers and use the same scheme, frame count, and port as the receiver. The simulation starts after all stations connect.

## Send data between two stations

Build with `make`. For the simplest run, use three separate terminals:

```bash
./receiver_app 2 cd
./sender_app 0 cd
./sender_app 1 cd
```

For custom messages and timing, run these in three separate terminals:

```bash
# Receiver: 2 stations, CSMA/CD, 1 frame each, port 8080, Tx=4, propagation=1.
./receiver_app 2 cd 1 8080 4 1
# Station 0 sends to station 1.
./sender_app 0 cd 1 0.5 8080 1 1 "Hello from station 0"
# Station 1 sends to station 0.
./sender_app 1 cd 1 0.5 8080 1 0 "Hello from station 1"
```

A destination of `-1` keeps the original channel-only demonstration. An addressed frame is forwarded through the channel only after successful transmission. The destination prints `received: source frame message`. Both processes stay connected until all frames are received, so a station that has finished sending can still receive data. If there are multiple frames, each carries the same supplied message.

The receiver's last two arguments are transmission time and one-way propagation time, in slots. With `4 1`, the detection condition `4 >= 2 * 1` holds. Restart the example with `1 1` to test a transmission too short for this worst-case detection rule: colliding frames are discarded and retried after `NO_ACK`, without a collision-detection result. Delivery itself has no extra propagation delay in this simple model.

## Files

- `CSMA.cpp`: socket helpers, carrier sensing, collision detection, backoff, station behavior, and shared channel.
- `sender.cpp`: starts one station.
- `receiver.cpp`: starts the channel and receives frames.
- `evaluate.cpp`: starts independent stations, measures performance, saves CSV files, and draws SVG plots.

## Reading CSMA.cpp

Each scheme has its own small function: `nonPersistent()`, `onePersistent()`, `pPersistent()`, and `csmaCd()`. `shouldTransmit()` handles waiting and calls the selected scheme. `handleResult()` handles success or collision, while `backoff()` chooses the retry wait.

`runStation()` connects, reads a result, decides whether to transmit, and sends its request. `runChannel()` calls `exchangeRequests()`, `startTransmission()`, and `finishSlot()` for each slot. Connection setup, shutdown, and metric calculation are separate functions. Every function has a short comment describing its purpose.

## Schemes and channel

- `non`: when busy, wait 1 to 8 random slots before sensing again.
- `one`: sense each slot and transmit immediately when idle.
- `p`: transmit with probability p when idle; otherwise wait one slot.
- `cd`: use 1-persistent sensing, abort collisions early, and use binary exponential backoff. After k collisions, choose 0 to 2^min(k,10) - 1 slots; reset k after success.

The first three schemes use a random backoff of 0 to 7 slots after collision. Backoff counts down in simulation slots; transmissions start only when idle.

Every slot, all stations sense the same channel state before requests are applied. Simultaneous requests collide. By default a frame takes four slots and one-way propagation time is one slot. CSMA/CD reports a collision after two propagation times only when transmission time is at least that long. Otherwise it reports `NO_ACK` at frame completion; the station retries with a random wait. The missing-ACK timeout is abstracted into this result, rather than implemented as a real timer. Other schemes report collisions after the full frame time. TCP carries messages; this slotted model applies a worst-case detection timing rule without simulating physical signals or Ethernet jam bits.

Each station starts with one ready numbered frame. Its next frame becomes ready after success. The receiver checks frame order and rejects extra frames. Completed stations stop contending, so N is the initial station count.

## Evaluation

```bash
./evaluate_app --sweep
```

Tests p = 0.1, 0.25, 0.5, 0.75, 1 at fixed N = 4. The tested p with the highest mean throughput is used to compare all schemes at N = 2, 4, 8, 12. Each setting uses three seeds and five frames per station (63 runs total).

Outputs in `output/`:

- `p_results.csv`, `N_results.csv`: measurements for each run.
- `p_collisions.svg`, `p_delay.svg`, `p_throughput.svg`: metrics against p.
- `N_collisions.svg`, `N_delay.svg`, `N_throughput.svg`: comparisons against N.
- `best_p.txt`: best tested p for the fixed-N workload.
- `received.csv`: station and frame numbers received in the last run.
- `run_results.csv`: metrics from a single evaluator run.

Open the SVG plots in a browser. Collisions count shared collision events. Delay is the mean slots from frame readiness to reception, including waiting and retries. Throughput is received frames / elapsed slots; its maximum is 0.25 frames/slot. Efficiency is throughput multiplied by four.

Random seeds control station decisions. Results are short finite-batch measurements, and the best tested p is specific to this workload. `make clean` removes executables. Generated results are excluded from Git.

## Observations from the C++ run

All 63 sweep runs completed with ordered, exactly-once frame delivery. At fixed N = 4, p = 0.25 had the highest mean throughput among the tested values (0.159 frames/slot). At p = 1, collisions increased to 32 per run and throughput dropped to 0.090. A very small p reduced collisions but left more slots idle.

Increasing N caused severe contention for 1-persistent CSMA: its mean throughput fell from 0.145 at N = 2 to 0.004 at N = 12. Non-persistent waiting reduced repeated collisions. CSMA/CD saves collision time when the transmission duration meets the detection condition, but exponential backoff can also introduce idle slots. Compare the regenerated plots for the current timing model; no scheme is guaranteed to dominate every workload.

Short commands to send only to the common receiver (one command per terminal):

./receiver_app 2 cd
./sender_app 0 cd -1
./sender_app 1 cd -1

A lone -1 after the scheme disables forwarding and uses five frames with
all other defaults. Without -1, stations 0 and 1 send to each other.
In the full positional command, the first optional positive number still
means frames per station; the destination argument can also be set to -1.
