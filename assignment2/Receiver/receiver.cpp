#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "../Sender/Frame.cpp"
#include "../CommonFunctions/SocketConnection.cpp"

using namespace std;
using boost::asio::ip::tcp;

class ReceiverProgram {
private:
    SocketConnection& connection;
    string protocol;
    int windowSize;
    double errorProbability;
    mt19937 randomGenerator;
    uniform_real_distribution<double> probability;
    int expectedSequence;
    int receiveBase;
    map<int, string> selectiveBuffer;
    vector<string> acceptedPayloads;

    // Store payloads in delivery order.
    void acceptPayload(string payload) {
        acceptedPayloads.push_back(payload);
    }

    void receiveStopAndWait(int sequenceNumber, string payload) {
        if (sequenceNumber == expectedSequence) {
            acceptPayload(payload);
            expectedSequence++;
            Send(sequenceNumber);
        }
        else if (sequenceNumber < expectedSequence) {
            // The original ACK may have been lost, so ACK a duplicate again.
            Send(sequenceNumber);
        }
    }

    void receiveGoBackN(int sequenceNumber, string payload) {
        if (sequenceNumber == expectedSequence) {
            acceptPayload(payload);
            expectedSequence++;
        }

        // This is cumulative: the highest in-order number is ACKed.
        Send(expectedSequence - 1);
    }

    void receiveSelectiveRepeat(int sequenceNumber, string payload) {
        if (sequenceNumber >= receiveBase &&
            sequenceNumber < receiveBase + windowSize) {
            if (selectiveBuffer.count(sequenceNumber) == 0) {
                selectiveBuffer[sequenceNumber] = payload;
            }

            // Independent acknowledgement, even for out-of-order frames.
            Send(sequenceNumber);

            while (selectiveBuffer.count(receiveBase) == 1) {
                acceptPayload(selectiveBuffer[receiveBase]);
                selectiveBuffer.erase(receiveBase);
                receiveBase++;
            }
        }
        else if (sequenceNumber < receiveBase) {
            Send(sequenceNumber);
        }
    }

public:
    ReceiverProgram(SocketConnection& selectedConnection,
                    string selectedProtocol,
                    int selectedWindow,
                    double selectedProbability,
                    int seed)
        : connection(selectedConnection),
          randomGenerator(seed), probability(0.0, 1.0) {
        protocol = selectedProtocol;
        windowSize = selectedWindow;
        errorProbability = selectedProbability;
        expectedSequence = 0;
        receiveBase = 0;
    }

    // Check(): reject malformed or CRC-corrupted data frames.
    bool Check(string frameBits) {
        return (int)frameBits.length() == FRAME_SIZE_BITS &&
               detectCRC32(frameBits);
    }

    // Recv(): validate and process one DATA frame.
    void Recv(string frameBits) {
        if (!Check(frameBits)) {
            cout << "Discarded a corrupted frame\n";
            return;
        }

        int sequenceNumber = readSequenceNumber(frameBits);
        string payload = readPayload(frameBits);
        cout << "Received valid frame " << sequenceNumber << '\n';

        if (protocol == "STOP_WAIT") {
            receiveStopAndWait(sequenceNumber, payload);
        }
        else if (protocol == "GO_BACK_N") {
            receiveGoBackN(sequenceNumber, payload);
        }
        else {
            receiveSelectiveRepeat(sequenceNumber, payload);
        }
    }

    // Send(): pass one acknowledgement through an impaired ACK channel.
    void Send(int ackNumber) {
        if (ackNumber < 0) {
            return;
        }

        this_thread::sleep_for(
            chrono::milliseconds(5 + randomGenerator() % 16)
        );

        if (probability(randomGenerator) < errorProbability) {
            int impairment = randomGenerator() % 3;

            if (impairment == 0) {
                cout << "ACK " << ackNumber << " lost\n";
                return;
            }
            if (impairment == 1) {
                cout << "ACK " << ackNumber << " corrupted\n";
                connection.sendLine("ACK BAD");
                return;
            }

            // Delayed ACKs can arrive after the sender has timed out.
            cout << "ACK " << ackNumber << " excessively delayed\n";
            this_thread::sleep_for(chrono::milliseconds(450));
        }

        connection.sendLine("ACK " + to_string(ackNumber));
    }

    bool writeOutput(string outputFileName) {
        ofstream outputFile(outputFileName, ios::binary);

        if (!outputFile.is_open()) {
            return false;
        }

        for (int i = 0; i < (int)acceptedPayloads.size(); i++) {
            outputFile.write(
                acceptedPayloads[i].data(), acceptedPayloads[i].size()
            );
        }

        return true;
    }
};

int main(int argc, char* argv[]) {
    int portNumber = argc >= 2 ? stoi(argv[1]) : 8080;
    string outputFileName = argc >= 3 ? argv[2] : "received_output.txt";

    if (portNumber < 1 || portNumber > 65535) {
        cerr << "Error: invalid port number\n";
        return 1;
    }

    try {
        boost::asio::io_context ioContext;
        tcp::endpoint endpoint(tcp::v4(), portNumber);
        tcp::acceptor acceptor(ioContext);
        acceptor.open(endpoint.protocol());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();
        cout << "Receiver waiting on port " << portNumber << "...\n";

        tcp::socket socket(ioContext);
        acceptor.accept(socket);
        SocketConnection connection(socket);
        string line;

        if (!connection.receiveLine(line, 30000)) {
            cerr << "Error: START message was not received\n";
            return 1;
        }

        istringstream startInput(line);
        string messageType;
        string protocol;
        int windowSize;
        double errorProbability;
        int seed;
        startInput >> messageType >> protocol >> windowSize;
        startInput >> errorProbability >> seed;

        if (messageType != "START" || startInput.fail()) {
            cerr << "Error: invalid START message\n";
            return 1;
        }

        ReceiverProgram receiver(
            connection, protocol, windowSize, errorProbability, seed
        );

        while (connection.receiveLine(line, 30000)) {
            if (line == "END") {
                if (!receiver.writeOutput(outputFileName)) {
                    cerr << "Error: could not write receiver output\n";
                    return 1;
                }

                cout << "Output written to " << outputFileName << '\n';
                return 0;
            }

            if (line.rfind("DATA ", 0) == 0) {
                receiver.Recv(line.substr(5));
            }
        }

        cerr << "Error: connection ended before END message\n";
        return 1;
    }
    catch (exception& error) {
        cerr << "Receiver socket error: " << error.what() << '\n';
        return 1;
    }
}
