#include <string>

using namespace std;

// Every frame can store 48 payload bytes (384 bits).
const int PAYLOAD_SIZE_BYTES = 48;
const int PAYLOAD_SIZE_BITS = PAYLOAD_SIZE_BYTES * 8;
const int HEADER_SIZE = 128;

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
    const int payloadLength = PAYLOAD_SIZE_BYTES;
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

        // Store the selected method as a fixed 16-bit code.
        if (errorDetectionType == "CHECKSUM16") {
            errorDetectionCode = 1;
        }
        else if (errorDetectionType == "CRC8") {
            errorDetectionCode = 2;
        }
        else if (errorDetectionType == "CRC10") {
            errorDetectionCode = 3;
        }
        else if (errorDetectionType == "CRC16") {
            errorDetectionCode = 4;
        }
        else if (errorDetectionType == "CRC32") {
            errorDetectionCode = 5;
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
    // Store exactly 384 bits and add zero padding when needed.
    void storeBits(string payloadBits) {
        if (payloadBits.length() > PAYLOAD_SIZE_BITS) {
            payloadBits = payloadBits.substr(0, PAYLOAD_SIZE_BITS);
        }

        // Add one zero at a time until 384 bits are stored.
        while (payloadBits.length() < PAYLOAD_SIZE_BITS) {
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
    string errorDetectionBits;

public:
    void storeErrorDetectionBits(string bits) {
        errorDetectionBits = bits;
    }

    string getErrorDetectionBits() {
        return errorDetectionBits;
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

    void storeErrorDetectionBits(string bits) {
        trailer.storeErrorDetectionBits(bits);
    }

    string getErrorDetectionBits() {
        return trailer.getErrorDetectionBits();
    }

    // Combine the complete frame before transmission.
    string getCompleteFrameBits() {
        return getHeaderBits() + getPayloadBits() + getErrorDetectionBits();
    }
};
