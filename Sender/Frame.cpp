#include <string>

using namespace std;

// Every frame can store 48 payload bits.
const int PAYLOAD_SIZE = 48;

// Convert a number into a fixed number of bits.
string numberToBits(long long number, int totalBits) {
    string bits = "";

    for (int i = 0; i < totalBits; i++) {
        if (number % 2 == 0) {
            bits = '0' + bits;
        }
        else {
            bits = '1' + bits;
        }

        number = number / 2;
    }

    return bits;
}

// Stores information placed at the beginning of a frame.
class Header {
private:
    // Temporary MAC addresses until MAC generation is added.
    const long long sourceMac = 123456789012;
    const long long destinationMac = 210987654321;
    const int payloadLength = PAYLOAD_SIZE;
    string errorDetectionType;

public:
    void storeErrorDetectionType(string type) {
        errorDetectionType = type;
    }

    string getErrorDetectionType() {
        return errorDetectionType;
    }

    // Convert all header fields into bits.
    string getBits() {
        string bits = "";
        int errorDetectionCode = 0;

        // For now, CHECKSUM16 is the only available type.
        if (errorDetectionType == "CHECKSUM16") {
            errorDetectionCode = 1;
        }

        bits += numberToBits(sourceMac, 48);
        bits += numberToBits(destinationMac, 48);
        bits += numberToBits(payloadLength, 16);
        bits += numberToBits(errorDetectionCode, 16);

        return bits;
    }
};

// Stores the actual data carried by a frame.
class Payload {
private:
    string bits;

public:
    // Store exactly 48 bits and add zero padding when needed.
    void storeBits(string payloadBits) {
        if (payloadBits.length() > PAYLOAD_SIZE) {
            payloadBits = payloadBits.substr(0, PAYLOAD_SIZE);
        }

        // Add one zero at a time until 48 bits are stored.
        while (payloadBits.length() < PAYLOAD_SIZE) {
            payloadBits += '0';
        }

        bits = payloadBits;
    }

    string getBits() {
        return bits;
    }
};

// Stores information placed at the end of a frame.
class Trailer {
private:
    string checksumBits;

public:
    void storeChecksum(string checksum) {
        checksumBits = checksum;
    }

    string getChecksum() {
        return checksumBits;
    }
};

// A complete frame contains a header, payload, and trailer.
class Frame {
private:
    Header header;
    Payload payload;
    Trailer trailer;

public:
    // Create a frame with its payload and error-detection type.
    Frame(string payloadBits, string errorDetectionType) {
        payload.storeBits(payloadBits);
        header.storeErrorDetectionType(errorDetectionType);
    }

    string getPayloadBits() {
        return payload.getBits();
    }

    string getHeaderBits() {
        return header.getBits();
    }

    string getErrorDetectionType() {
        return header.getErrorDetectionType();
    }

    void storeChecksum(string checksum) {
        trailer.storeChecksum(checksum);
    }

    string getChecksum() {
        return trailer.getChecksum();
    }
};
