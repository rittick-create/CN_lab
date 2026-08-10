#include <string>
#include <vector>

using namespace std;

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

        if (frameLength <= HEADER_SIZE + PAYLOAD_SIZE) {
            return false;
        }

        string headerBits = frameBits.substr(0, HEADER_SIZE);
        string payloadBits = frameBits.substr(HEADER_SIZE, PAYLOAD_SIZE);
        string trailerBits = frameBits.substr(HEADER_SIZE + PAYLOAD_SIZE);
        string errorDetectionType = readErrorDetectionType(headerBits);
        string generator = getCRCGenerator(errorDetectionType);
        bool frameIsValid = false;

        if (errorDetectionType == "") {
            return false;
        }

        if (errorDetectionType == "CHECKSUM16") {
            frameIsValid = detectChecksum16(
                headerBits, payloadBits, trailerBits
            );
        }
        else {
            frameIsValid = detectCRC(
                headerBits, payloadBits, trailerBits, generator
            );
        }

        if (frameIsValid == false) {
            return false;
        }
    }

    return true;
}
