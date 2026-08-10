#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include "../Sender/Frame.cpp"
#include "../ErrorDetection/Checksum/ChecksumDetection.cpp"
#include "../ErrorDetection/CRC/CRC.cpp"
#include "../ErrorDetection/CRC/CRCDetection.cpp"

using namespace std;
using boost::asio::ip::tcp;

// Convert the 16-bit method code from the header into a number.
int errorDetectionCodeToNumber(string codeBits) {
    int number = 0;

    for (int i = 0; i < 16; i++) {
        number = number * 2;

        if (codeBits[i] == '1') {
            number = number + 1;
        }
        else if (codeBits[i] != '0') {
            return 0;
        }
    }

    return number;
}

// Read the selected error-detection type from the header.
string readErrorDetectionType(string headerBits) {
    if (headerBits.length() != HEADER_SIZE) {
        return "";
    }

    string codeBits = headerBits.substr(HEADER_SIZE - 16, 16);
    int code = errorDetectionCodeToNumber(codeBits);

    if (code == 1) {
        return "CHECKSUM16";
    }
    if (code == 2) {
        return "CRC8";
    }
    if (code == 3) {
        return "CRC10";
    }
    if (code == 4) {
        return "CRC16";
    }
    if (code == 5) {
        return "CRC32";
    }

    return "";
}

// Check all frames received from the sender.
bool receiveFrames(vector<string>& receivedFrames) {
    int totalFrames = receivedFrames.size();

    for (int i = 0; i < totalFrames; i++) {
        string frameBits = receivedFrames[i];
        int frameLength = frameBits.length();

        if (frameLength <= HEADER_SIZE + PAYLOAD_SIZE_BITS) {
            return false;
        }

        string headerBits = frameBits.substr(0, HEADER_SIZE);
        string payloadBits = frameBits.substr(HEADER_SIZE, PAYLOAD_SIZE_BITS);
        string trailerBits = frameBits.substr(
            HEADER_SIZE + PAYLOAD_SIZE_BITS
        );
        string errorDetectionType = readErrorDetectionType(headerBits);
        string generator = getCRCGenerator(errorDetectionType);
        bool frameIsValid = false;

        if (errorDetectionType == "") {
            return false;
        }

        if (errorDetectionType == "CHECKSUM16") {
            frameIsValid = detectChecksum16(payloadBits, trailerBits);
        }
        else {
            frameIsValid = detectCRC(
                payloadBits, trailerBits, generator
            );
        }

        if (frameIsValid == false) {
            return false;
        }
    }

    return true;
}

// Run the receiver as a TCP server.
int main(int argc, char* argv[]) {
    try {
        int portNumber = 8080;

        if (argc >= 2) {
            portNumber = stoi(argv[1]);
        }

        if (portNumber < 1 || portNumber > 65535) {
            cerr << "Error: invalid port number\n";
            return 1;
        }

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

        boost::asio::streambuf inputBuffer;
        istream inputStream(&inputBuffer);
        string line;

        // Read the number of incoming frames.
        boost::asio::read_until(socket, inputBuffer, '\n');
        getline(inputStream, line);
        int totalFrames = stoi(line);

        if (totalFrames <= 0) {
            cerr << "Error: no frames received\n";
            return 1;
        }

        vector<string> receivedFrames;

        // Read every complete frame.
        for (int i = 0; i < totalFrames; i++) {
            boost::asio::read_until(socket, inputBuffer, '\n');

            string frameBits;
            getline(inputStream, frameBits);
            receivedFrames.push_back(frameBits);
        }

        bool receiverResult = receiveFrames(receivedFrames);
        string response;

        if (receiverResult == true) {
            response = "true\n";
            cout << "Receiver result: true (no error detected)\n";
        }
        else {
            response = "false\n";
            cout << "Receiver result: false (error detected)\n";
        }

        boost::asio::write(socket, boost::asio::buffer(response));
        return 0;
    }
    catch (exception& error) {
        cerr << "Receiver socket error: " << error.what() << '\n';
        return 1;
    }
}
