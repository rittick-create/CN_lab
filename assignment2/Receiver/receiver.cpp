#include <boost/asio.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include "../Sender/Frame.cpp"
#include "../CommonFunctions/SocketConnection.cpp"

using namespace std;
using boost::asio::ip::tcp;

class ReceiverProgram {
private:
    SocketConnection& connection;
    string protocol;
    double errorProbability;
    mt19937 randomGenerator;
    uniform_real_distribution<double> probability;
    int expectedSequence;
    string receivedData;

    // Store payloads in delivery order.
    void acceptPayload(string payload) {
        receivedData += payload;
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

    // ACK-only demonstration: CRC was checked by Recv().
    void receiveSelectiveRepeat(int sequenceNumber) {
        Send(sequenceNumber);
    }

public:
    ReceiverProgram(SocketConnection& selectedConnection,
                    string selectedProtocol,
                    double selectedProbability)
        : connection(selectedConnection),
          randomGenerator(random_device{}()), probability(0.0, 1.0) {
        protocol = selectedProtocol;
        errorProbability = selectedProbability;
        expectedSequence = 0;
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
            receiveSelectiveRepeat(sequenceNumber);
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

        // The receiver creates the ACK text here and sends it to the sender.
        string message = "ACK " + to_string(ackNumber);
        connection.sendLine(message);
    }

    bool writeOutput(string outputFileName) {
        ofstream outputFile(outputFileName, ios::binary);

        if (!outputFile.is_open()) {
            return false;
        }

        outputFile.write(receivedData.data(), receivedData.size());
        return !outputFile.fail();
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
        startInput >> messageType >> protocol >> windowSize;
        startInput >> errorProbability;

        if (messageType != "START" || startInput.fail()) {
            cerr << "Error: invalid START message\n";
            return 1;
        }

        ReceiverProgram receiver(
            connection, protocol, errorProbability
        );

        // After START, each message is either DATA or END.
        while (connection.receiveLine(line, 30000)) {
            if (line == "END") {
                if (!receiver.writeOutput(outputFileName)) {
                    cerr << "Error: could not write receiver output\n";
                    return 1;
                }

                cout << "Output written to " << outputFileName << '\n';
                return 0;
            }

            if (line.substr(0, 5) == "DATA ") {
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
