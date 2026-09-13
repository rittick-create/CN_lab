#include <boost/asio.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Frame.cpp"
#include "Channel.cpp"
#include "../CommonFunctions/SocketConnection.cpp"

using namespace std;
using boost::asio::ip::tcp;

class SenderProgram {
private:
    SocketConnection& connection;
    DataChannel channel;
    vector<Frame> frames;
    vector<int> attempts;
    double estimatedRTT;
    double rttDeviation;
    int timeoutMilliseconds;
    int windowSize;
    string protocol;
    int transmissions;
    int retransmissions;
    int timeouts;
    vector<double> rttSamples;

    // Send one frame through the simulated channel and then the TCP socket.
    //return false if the frame exceeds 100 transmission attempts.
    //returns true if the freame is sent successfully or if it is dropped, corrupted, or delayed.
    bool transmitFrame(int frameNumber,
                       chrono::steady_clock::time_point& sentAt) {
        if (attempts[frameNumber] >= 100) {//if frame reached 100 attempths then print error and return false
            cerr << "Error: frame " << frameNumber;
            cerr << " exceeded 100 transmission attempts\n";
            return false;
        }
        //check whether it is a retransmiission
        bool isRetransmission = attempts[frameNumber] > 0;
        attempts[frameNumber]++;//update the attempts for the frame
        transmissions++;

        if (isRetransmission) {
            retransmissions++;
        }
        //frame is a vector ,so this one gets the bits of the whole frame of a particular frame number
        string transmittedBits =
            frames[frameNumber].getCompleteFrameBits();
        //record the time when the frame is sent    
        sentAt = chrono::steady_clock::now();
        //send the frame through the simulated channel and check if it reaches the receiver
        bool reachesReceiver = channel.Channel(
            transmittedBits, timeoutMilliseconds
        );

        cout << (isRetransmission ? "Retransmitting" : "Sending");
        cout << " frame " << frameNumber << '\n';

        if (!reachesReceiver) {
            return true;
        }

        return connection.sendLine("DATA " + transmittedBits);
    }
    //how much time is left before the timeout expires
    int millisecondsRemaining(
        chrono::steady_clock::time_point started,
        int allowedMilliseconds) {
        long elapsed = chrono::duration_cast<chrono::milliseconds>(
            chrono::steady_clock::now() - started
        ).count();
        return max(0, allowedMilliseconds - (int)elapsed);
    }
    //measusres rtt
    double elapsedMilliseconds(chrono::steady_clock::time_point started) {
        return chrono::duration<double, milli>(
            chrono::steady_clock::now() - started
        ).count();
    }

    // Run Stop-and-Wait ARQ.
    //for each frame =>while the frames is not ack=>tranmit the frame
    //while time ramins=>wait fro an ack=>check if the ack is for the frame=>if yes then break and go to the next frame
    //if not acknowledged=>increment the timeouts and print timeout for the frame
    bool runStopAndWait() {
        for (int frameNumber = 0;
             frameNumber < (int)frames.size();
             frameNumber++) {
            bool acknowledged = false;

            while (!acknowledged) {
                chrono::steady_clock::time_point sentAt;

                if (!transmitFrame(frameNumber, sentAt)) {
                    return false;//frame exeeded 100 attempts or socket failed
                }

                while (millisecondsRemaining(
                           sentAt, timeoutMilliseconds
                       ) > 0) {
                    int ackNumber = -1;
                    int waitTime = millisecondsRemaining(
                        sentAt, timeoutMilliseconds
                    );

                    if (!Recv(waitTime, ackNumber)) {//waits for a socket message for at most waitTime milliseconds.
                        break;
                    }

                    if (ackNumber == frameNumber) {
                        double rtt = elapsedMilliseconds(sentAt);
                        Timeout(rtt);
                        cout << "ACK " << ackNumber << " received, RTT = ";
                        cout << fixed << setprecision(2) << rtt << " ms\n";
                        acknowledged = true;
                        break;
                    }
                }

                if (!acknowledged) {
                    timeouts++;
                    cout << "Timeout for frame " << frameNumber << '\n';
                }
            }
        }

        return true;
    }

    //Go-Back-N with cumulative acknowledgements.
    bool runGoBackN() {
        int base = 0;//oldest frame that has not been acknowledged yet

        while (base < (int)frames.size()) {//Continue until all frames are acknowledged.
            // When base == frames.size() the transfer is complete

            int end = min(base + windowSize, (int)frames.size());//calculate the end of the current window

            vector<chrono::steady_clock::time_point> sentAt(frames.size());//tranmssion time for each frame

            for (int i = base; i < end; i++) {//send the complete win
                if (!transmitFrame(i, sentAt[i])) {
                    return false;
                }
            }

            chrono::steady_clock::time_point windowStarted = sentAt[base];//oldest unacknowledged frame's transmission time
            bool windowTimedOut = false;//indicate if the window has timed out

            while (base < end) {
                int waitTime = millisecondsRemaining(
                    windowStarted, timeoutMilliseconds
                );

                if (waitTime <= 0) {
                    windowTimedOut = true;
                    break;
                }

                int ackNumber = -1;

                if (!Recv(waitTime, ackNumber)) {
                    windowTimedOut = true;
                    break;
                }

                // ACK i cumulatively acknowledges every frame through i.
                if (ackNumber >= base && ackNumber < end) {
                    double rtt = elapsedMilliseconds(sentAt[ackNumber]);
                    Timeout(rtt);
                    base = ackNumber + 1;
                    cout << "Cumulative ACK " << ackNumber;
                    cout << " received; base = " << base << '\n';
                }
            }

            if (windowTimedOut && base < end) {
                timeouts++;
                cout << "Window timeout; Go-Back-N restarts at frame ";
                cout << base << '\n';
            }
        }

        return true;
    }

    // Run Selective Repeat with an independent timer and ACK per frame.
    //send all the new frames in the window
    //wait for an ack
    //mark only that fram as ack
    //check every fram  fro timeout
    //retransmitt only the timeout ones
    //move the window forward
    bool runSelectiveRepeat() {
        int totalFrames = frames.size();
        int base = 0;
        vector<bool> acknowledged(totalFrames, false);
        vector<chrono::steady_clock::time_point> sentAt(totalFrames);

        while (base < totalFrames) {
            int end = min(base + windowSize, totalFrames);

            // Send frames that have not been sent before.
            for (int i = base; i < end; i++) {
                if (attempts[i] == 0) {
                    if (!transmitFrame(i, sentAt[i])) {
                        return false;
                    }
                }
            }

            // Wait briefly for one acknowledgement.
            int ackNumber = -1;

            if (Recv(20, ackNumber) &&//waits for 20ms for ack
                ackNumber >= base && ackNumber < end &&
                !acknowledged[ackNumber]) {
                acknowledged[ackNumber] = true;
                double rtt = elapsedMilliseconds(sentAt[ackNumber]);
                Timeout(rtt);
                cout << "ACK " << ackNumber << " received\n";
            }

            // Check each unacknowledged frame for a timeout.
            for (int i = base; i < end; i++) {
                if (!acknowledged[i]) {
                    double elapsed = elapsedMilliseconds(sentAt[i]);

                    if (elapsed >= timeoutMilliseconds) {
                        timeouts++;
                        cout << "Timeout for frame " << i << '\n';

                        // Retransmit only the frame whose timer expired.
                        if (!transmitFrame(i, sentAt[i])) {
                            return false;
                        }
                    }
                }
            }

            // Move past continuously acknowledged frames.
            while (base < totalFrames && acknowledged[base]) {
                base++;
            }
        }

        return true;
    }

public:
    SenderProgram(SocketConnection& selectedConnection,
                  string selectedProtocol,
                  int selectedWindow,
                  double errorProbability,
                  int seed)
        : connection(selectedConnection),
          channel(errorProbability, seed) {
        protocol = selectedProtocol;
        windowSize = selectedWindow;
        estimatedRTT = 200.0;
        rttDeviation = 50.0;
        timeoutMilliseconds = 400;
        transmissions = 0;
        retransmissions = 0;
        timeouts = 0;
    }

    // Framing(): split a binary-safe input file into fixed-size payloads.
    bool Framing(string inputFileName) {
        ifstream inputFile(inputFileName, ios::binary);

        if (!inputFile.is_open()) {
            cerr << "Error: could not open " << inputFileName << '\n';
            return false;
        }

        string contents(
            (istreambuf_iterator<char>(inputFile)),
            istreambuf_iterator<char>()
        );

        if (contents.empty()) {
            cerr << "Error: input file is empty\n";
            return false;
        }

        for (int position = 0;
             position < (int)contents.length();
             position += PAYLOAD_SIZE_BYTES) {
            string payload = contents.substr(position, PAYLOAD_SIZE_BYTES);
            frames.push_back(Frame(payload, frames.size()));
        }

        if (frames.size() > 256) {
            cerr << "Error: the 1-byte sequence field supports at most ";
            cerr << 256 << " frames in this simulation\n";
            return false;
        }

        attempts.assign(frames.size(), 0);
        return true;
    }

    // Timer(): return the current adaptive timeout value.
    int Timer() {
        return timeoutMilliseconds;
    }

    // Timeout(): update timeout = estimated RTT + 4 * RTT deviation.
    void Timeout(double recentRTT) {
        double error = abs(estimatedRTT - recentRTT);
        estimatedRTT = 0.875 * estimatedRTT + 0.125 * recentRTT;
        rttDeviation = 0.75 * rttDeviation + 0.25 * error;
        timeoutMilliseconds = (int)(estimatedRTT + 4 * rttDeviation);
        timeoutMilliseconds = max(100, min(timeoutMilliseconds, 2000));
        rttSamples.push_back(recentRTT);
    }

    // Recv(): wait for and decode one ACK packet.
    bool Recv(int waitMilliseconds, int& ackNumber) {
        string line;

        if (!connection.receiveLine(line, waitMilliseconds)) {
            return false;
        }

        istringstream input(line);
        string type;
        input >> type >> ackNumber;
        return type == "ACK" && !input.fail();
    }

    // Send(): choose and run the requested flow-control scheme.
    bool Send() {
        if (protocol == "STOP_WAIT") {
            return runStopAndWait();
        }
        if (protocol == "GO_BACK_N") {
            return runGoBackN();
        }
        if (protocol == "SELECTIVE_REPEAT") {
            return runSelectiveRepeat();
        }

        return false;
    }

    void printStatistics(double totalMilliseconds) {
        double averageRTT = 0.0;

        for (int i = 0; i < (int)rttSamples.size(); i++) {
            averageRTT += rttSamples[i];
        }

        if (!rttSamples.empty()) {
            averageRTT = averageRTT / rttSamples.size();
        }

        double efficiency = 100.0 * frames.size() / transmissions;

        cout << "\nTransmission complete\n";
        cout << "Protocol: " << protocol << '\n';
        cout << "Frames: " << frames.size() << '\n';
        cout << "Transmissions: " << transmissions << '\n';
        cout << "Retransmissions: " << retransmissions << '\n';
        cout << "Timeouts: " << timeouts << '\n';
        cout << fixed << setprecision(2);
        cout << "Average RTT: " << averageRTT << " ms\n";
        cout << "Final timeout: " << timeoutMilliseconds << " ms\n";
        cout << "Efficiency: " << efficiency << "%\n";
        cout << "Total time: " << totalMilliseconds << " ms\n";

        // Machine-readable values used by Evaluation/evaluate.py.
        cout << "STAT protocol=" << protocol << '\n';
        cout << "STAT frames=" << frames.size() << '\n';
        cout << "STAT transmissions=" << transmissions << '\n';
        cout << "STAT retransmissions=" << retransmissions << '\n';
        cout << "STAT timeouts=" << timeouts << '\n';
        cout << "STAT average_rtt_ms=" << averageRTT << '\n';
        cout << "STAT final_timeout_ms=" << timeoutMilliseconds << '\n';
        cout << "STAT efficiency_percent=" << efficiency << '\n';
        cout << "STAT total_time_ms=" << totalMilliseconds << '\n';
        cout << "STAT data_drops=" << channel.droppedFrames << '\n';
        cout << "STAT data_corruptions=" << channel.corruptedFrames << '\n';
        cout << "STAT excessive_delays=" << channel.delayedFrames << '\n';
    }
};

string normalizeProtocol(string value) {
    if (value == "stopwait" || value == "stop-and-wait") {
        return "STOP_WAIT";
    }
    if (value == "gobackn" || value == "go-back-n") {
        return "GO_BACK_N";
    }
    if (value == "selective" || value == "selective-repeat") {
        return "SELECTIVE_REPEAT";
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0];
        cerr << " <stopwait|gobackn|selective> <input_file>";
        cerr << " [window] [probability] [address] [port] [seed]\n";
        return 1;
    }

    string protocol = normalizeProtocol(argv[1]);
    string inputFileName = argv[2];
    int windowSize = argc >= 4 ? stoi(argv[3]) : 4;
    double errorProbability = argc >= 5 ? stod(argv[4]) : 0.0;
    string address = argc >= 6 ? argv[5] : "127.0.0.1";
    string port = argc >= 7 ? argv[6] : "8080";
    int seed = argc >= 8 ? stoi(argv[7]) : 42;

    if (protocol == "" || windowSize < 1 || windowSize > 128 ||
        errorProbability < 0.0 || errorProbability > 1.0) {
        cerr << "Error: invalid protocol, window, or probability\n";
        return 1;
    }

    if (protocol == "STOP_WAIT") {
        windowSize = 1;
    }

    try {
        boost::asio::io_context ioContext;
        tcp::resolver resolver(ioContext);
        tcp::socket socket(ioContext);
        boost::asio::connect(socket, resolver.resolve(address, port));
        SocketConnection connection(socket);

        string startMessage = "START " + protocol + " ";
        startMessage += to_string(windowSize) + " ";
        startMessage += to_string(errorProbability) + " ";
        startMessage += to_string(seed + 1000);

        if (!connection.sendLine(startMessage)) {
            cerr << "Error: could not start receiver session\n";
            return 1;
        }

        SenderProgram sender(
            connection, protocol, windowSize, errorProbability, seed
        );

        if (!sender.Framing(inputFileName)) {
            return 1;
        }

        chrono::steady_clock::time_point started =
            chrono::steady_clock::now();

        if (!sender.Send()) {
            cerr << "Error: transmission failed\n";
            return 1;
        }

        connection.sendLine("END");
        double totalMilliseconds = chrono::duration<double, milli>(
            chrono::steady_clock::now() - started
        ).count();
        sender.printStatistics(totalMilliseconds);
        return 0;
    }
    catch (exception& error) {
        cerr << "Sender socket error: " << error.what() << '\n';
        return 1;
    }
}
