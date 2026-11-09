#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

// Checks whether an operation succeeded.
// Throws an error with the given message if the condition is false.
inline void check(bool ok, string message) {
    if (!ok) throw runtime_error(message);
}

// Sets a 30-second timeout for sending and receiving socket data.
// Ignores SIGPIPE so a disconnected socket reports an error instead of ending the process.
inline void configure(int socket) {
    signal(SIGPIPE, SIG_IGN);
    timeval timeout{30, 0};
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

// Adds a newline and sends one complete text message through the socket.
// Repeats send() if only part of the message was sent.
inline void sendLine(int socket, string line) {
    line += '\n';
    size_t sent = 0;
    while (sent < line.size()) {
        ssize_t count = send(socket, line.data() + sent, line.size() - sent, 0);
        check(count > 0, "Could not send message");
        sent += count;
    }
}

// Reads one character at a time until a newline marks the end of the message.
// Returns the text, or reports a closed connection, timeout, or oversized message.
inline string receiveLine(int socket) {
    string line;
    char c;
    while (true) {
        check(recv(socket, &c, 1, 0) == 1, "Connection closed or timed out");
        if (c == '\n') return line;
        line += c;
        check(line.size() < 1024, "Message too long");
    }
}

// Creates a TCP listening socket on localhost for the shared channel.
// If port is zero, the OS chooses a free port and this function updates port.
// Returns the listening socket used to accept station connections.
inline int listenChannel(int& port) {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    check(listener >= 0, "Could not create socket");
    configure(listener);
    int reuse = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    check(::bind(listener, (sockaddr*)&address, sizeof(address)) == 0, "Could not bind port");
    check(listen(listener, 128) == 0, "Could not listen");
    socklen_t size = sizeof(address);
    check(getsockname(listener, (sockaddr*)&address, &size) == 0, "Could not get port");
    port = ntohs(address.sin_port);
    return listener;
}

// Checks that the selected scheme is non, one, p, or cd.
// Returns false for an unsupported scheme name.
inline bool validScheme(string scheme) {
    return scheme == "non" || scheme == "one" || scheme == "p" || scheme == "cd";
}

// Checks the channel state reported by the receiver.
// Returns true only when the channel is IDLE and a transmission may start.
inline bool carrierSense(string state) {
    return state == "IDLE";
}

// Checks the result of the previous transmission attempt.
// Returns true when the channel reports a COLLISION.
inline bool collisionDetected(string result) {
    return result == "COLLISION";
}

// Chooses a random wait after a collision: 0 to 7 slots for ordinary CSMA.
// CSMA/CD doubles the range after each collision, up to 1024 choices.
inline int backoff(mt19937& rng, int collisions, string scheme) {
    int limit = 8;
    if (scheme == "cd") {
        limit = 1;
        for (int i = 0; i < min(collisions, 10); i++) {
            limit *= 2;
        }
    }
    return uniform_int_distribution<int>(0, limit - 1)(rng);
}

// Non-persistent CSMA transmits immediately when the channel is idle.
// When busy, it chooses a random wait before sensing again.
inline bool nonPersistent(string state, int& wait, mt19937& rng) {
    if (carrierSense(state)) {
        return true;
    }
    wait = uniform_int_distribution<int>(1, 8)(rng);
    return false;
}

// 1-persistent CSMA transmits as soon as it senses an idle channel.
// When busy, it stays silent and senses again in the next slot.
inline bool onePersistent(string state) {
    return carrierSense(state);
}

// p-persistent CSMA transmits with probability p when the channel is idle.
// Otherwise, it stays silent and tries again in the next slot.
inline bool pPersistent(string state, double p, mt19937& rng) {
    if (!carrierSense(state)) {
        return false;
    }
    uniform_real_distribution<double> probability(0, 1);
    return probability(rng) < p;
}

// CSMA/CD uses 1-persistent sensing to decide when to start transmission.
// backoff() chooses the retry wait; startTransmission() checks the collision detection timing.
inline bool csmaCd(string state) {
    return onePersistent(state);
}

// Creates a station socket and connects it to the local shared channel.
// Sends the station number and returns the connected socket.
inline int connectStation(int port, int station) {
    int connection = socket(AF_INET, SOCK_STREAM, 0);
    check(connection >= 0, "Could not create station socket");
    configure(connection);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(port);
    check(connect(connection, (sockaddr*)&address, sizeof(address)) == 0,
          "Could not connect");
    sendLine(connection, to_string(station));
    return connection;
}

struct StationProgress {
    int completed = 0;
    int collisions = 0;
    int wait = 0;
};

// Advances to the next frame after success and resets the collision count.
// After collision detection or a missing ACK, keeps the frame and chooses a retry wait.
inline void handleResult(string result, StationProgress& progress,
                         string scheme, mt19937& rng) {
    if (result == "SUCCESS") {
        progress.completed++;
        progress.collisions = 0;
    } else if (result == "NO_ACK") {
        // A failed frame without collision detection is retried after an ACK timeout.
        progress.wait = uniform_int_distribution<int>(0, 7)(rng);
    } else if (collisionDetected(result)) {
        progress.collisions++;
        progress.wait = backoff(rng, progress.collisions, scheme);
    }
}

// Keeps completed stations and stations still waiting from transmitting.
// Calls the selected CSMA function when the station is ready to try.
inline bool shouldTransmit(string state, string scheme, int frames, double p,
                           StationProgress& progress, mt19937& rng) {
    if (progress.completed >= frames) {
        return false;
    }
    if (progress.wait > 0) {
        progress.wait--;
        return false;
    }
    if (scheme == "non") {
        return nonPersistent(state, progress.wait, rng);
    }
    if (scheme == "one") {
        return onePersistent(state);
    }
    if (scheme == "p") {
        return pPersistent(state, p, rng);
    }
    if (scheme == "cd") {
        return csmaCd(state);
    }
    throw runtime_error("Unknown CSMA scheme");
}

// Runs one station: reads channel messages, handles results, and requests transmission.
// Also displays data addressed to it by another station.
// Uses the selected CSMA rule and continues until the receiver sends STOP.
inline void runStation(int port, int station, string scheme, int frames, double p, int seed,
                       int destination = -1, string data = "") {
    int connection = connectStation(port, station);
    mt19937 rng(seed + station);
    StationProgress progress;
    while (true) {
        string line = receiveLine(connection);
        if (line == "STOP") {
            break;
        }
        if (line.rfind("DATA ", 0) == 0) {
            cout << "Station " << station << " received: " << line.substr(5) << endl;
            continue; // Data delivery is separate from the next channel tick.
        }
        string state, result;
        istringstream message(line);
        check(bool(message >> state >> result), "Invalid channel message");
        handleResult(result, progress, scheme, rng);
        bool transmit = shouldTransmit(state, scheme, frames, p, progress, rng);
        sendLine(connection, to_string(transmit) + " " + to_string(progress.completed)
                 + " " + to_string(destination) + " " + data);
    }
    close(connection);
}

struct Metrics {
    int collisions = 0, slots = 0, received = 0;
    double delay = 0, throughput = 0, efficiency = 0;
};

// The receiver keeps this information for each connected station.
struct ConnectedStation {
    int connection = -1;
    int id = 0;
    int completed = 0;
    int generated = 0;
    bool transmit = false;
    string result = "NONE";
    int destination = -1;
    string data;
};

// The current channel state and stations involved in a transmission.
struct Channel {
    string state = "IDLE";
    int remaining = 0;
    vector<int> active;
    int transmissionTime = 4;
    int propagationTime = 1;
    bool collided = false;
    bool detected = false;
};

// Waits for every station to connect and reads each station's number.
// Rejects duplicate numbers and returns the connected stations.
inline vector<ConnectedStation> acceptStations(int listener, int n) {
    vector<ConnectedStation> stations;
    for (int i = 0; i < n; i++) {
        ConnectedStation station;
        station.connection = accept(listener, nullptr, nullptr);
        check(station.connection >= 0, "Could not accept station");
        configure(station.connection);
        station.id = stoi(receiveLine(station.connection));
        for (const ConnectedStation& other : stations) {
            check(other.id != station.id, "Duplicate station number");
        }
        stations.push_back(station);
    }
    return stations;
}

// Sends the same channel state to all stations before reading their requests.
// Checks that each request names the expected frame, then clears the old result.
inline void exchangeRequests(vector<ConnectedStation>& stations, string state, int frames) {
    for (const ConnectedStation& station : stations) {
        sendLine(station.connection, state + " " + station.result);
    }
    for (ConnectedStation& station : stations) {
        int transmit, frame;
        istringstream request(receiveLine(station.connection));
        check(bool(request >> transmit >> frame >> station.destination), "Invalid station message");
        getline(request >> ws, station.data);
        if (transmit && station.destination >= 0) {
            bool found = false;
            for (const ConnectedStation& target : stations) {
                if (target.id == station.destination) found = true;
            }
            check(found, "Destination station is not connected");
        }
        check(frame == station.completed, "Wrong frame number");
        check(!transmit || station.completed < frames, "Extra frame");
        station.transmit = transmit != 0;
        station.result = "NONE";
    }
}

// On an idle channel, collects the stations that want to transmit.
// Multiple stations collide; CSMA/CD detects it only if transmissionTime >= 2 * propagationTime.
inline void startTransmission(Channel& channel, const vector<ConnectedStation>& stations,
                              string scheme, Metrics& metrics) {
    if (!carrierSense(channel.state)) {
        return;
    }
    channel.active.clear();
    for (int i = 0; i < int(stations.size()); i++) {
        if (stations[i].transmit) {
            channel.active.push_back(i);
        }
    }
    if (channel.active.empty()) {
        return;
    }
    channel.remaining = channel.transmissionTime;
    channel.collided = channel.active.size() > 1;
    channel.detected = false;
    channel.state = "BUSY";
    if (channel.collided) {
        metrics.collisions++;
        if (scheme == "cd") {
            channel.detected = channel.transmissionTime >= 2 * channel.propagationTime;
            if (channel.detected) channel.remaining = 2 * channel.propagationTime;
        } else {
            channel.detected = true;
            channel.state = "COLLISION";
        }
    }
}

// Advances a transmission by one slot and reports its result when it finishes.
// Records successful frames and delay, then makes the channel idle again.
inline void finishSlot(Channel& channel, vector<ConnectedStation>& stations,
                       int slot, Metrics& metrics, ostream& deliveries) {
    if (channel.remaining == 0) {
        return;
    }
    channel.remaining--;
    if (channel.remaining > 0) {
        return;
    }
    for (int i : channel.active) {
        ConnectedStation& station = stations[i];
        if (!channel.collided) {
            station.result = "SUCCESS";
            metrics.delay += slot + 1 - station.generated;
            station.generated = slot + 1;
            deliveries << station.id << ',' << station.completed << '\n';
            for (const ConnectedStation& target : stations) {
                if (target.id == station.destination) {
                    sendLine(target.connection, "DATA " + to_string(station.id) + " "
                             + to_string(station.completed) + " " + station.data);
                }
            }
            station.completed++;
            metrics.received++;
        } else {
            station.result = channel.detected ? "COLLISION" : "NO_ACK";
        }
    }
    channel.state = "IDLE";
}

// Tells every station to stop and closes their connections.
// Also closes the channel's listening socket.
inline void stopStations(const vector<ConnectedStation>& stations, int listener) {
    for (const ConnectedStation& station : stations) {
        sendLine(station.connection, "STOP");
        close(station.connection);
    }
    close(listener);
}

// Converts total delay into average delay and calculates frames per slot.
// Multiplies throughput by frame transmission time to get channel efficiency.
inline void calculateMetrics(Metrics& metrics, int transmissionTime) {
    metrics.delay /= metrics.received;
    metrics.throughput = double(metrics.received) / metrics.slots;
    metrics.efficiency = metrics.throughput * transmissionTime;
}

// Runs the channel by exchanging requests and advancing one slot at a time.
// Stops after receiving every frame, then returns the final measurements.
inline Metrics runChannel(int listener, int n, string scheme, int frames, ostream& deliveries,
                          int transmissionTime = 4, int propagationTime = 1) {
    vector<ConnectedStation> stations = acceptStations(listener, n);
    Channel channel;
    channel.transmissionTime = transmissionTime;
    channel.propagationTime = propagationTime;
    Metrics metrics;
    deliveries << "station,frame\n";
    for (int slot = 0; slot < 100000; slot++) {
        exchangeRequests(stations, channel.state, frames);
        startTransmission(channel, stations, scheme, metrics);
        finishSlot(channel, stations, slot, metrics, deliveries);
        metrics.slots = slot + 1;
        if (metrics.received == n * frames) {
            break;
        }
    }
    stopStations(stations, listener);
    check(metrics.received == n * frames, "Simulation exceeded 100000 slots");
    calculateMetrics(metrics, transmissionTime);
    return metrics;
}

// Prints the frame count, elapsed slots, and collision count.
// Also prints average delay and throughput.
inline void printMetrics(Metrics m) {
    cout << "Frames: " << m.received << "\nSlots: " << m.slots
         << "\nCollisions: " << m.collisions << "\nAverage delay: " << m.delay
         << " slots/frame\nThroughput: " << m.throughput
         << " frames/slot\n";
}
