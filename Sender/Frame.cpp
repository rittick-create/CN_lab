#include <string>

using namespace std;

// Every frame can store 48 payload bits.
const int PAYLOAD_SIZE = 48;

// Stores information placed at the beginning of a frame.
class Header {
private:
    // Temporary MAC addresses until MAC generation is added.
    const long long sourceMac = 123456789012;
    const long long destinationMac = 210987654321;
    const int payloadLength = PAYLOAD_SIZE;
    string errorDetectionType = "NONE";
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
    // Trailer fields will be added here.
};

// A complete frame contains a header, payload, and trailer.
class Frame {
private:
    Header header;
    Payload payload;
    Trailer trailer;

public:
    // Create a frame and place the given bits in its payload.
    Frame(string payloadBits) {
        payload.storeBits(payloadBits);
    }

    string getPayloadBits() {
        return payload.getBits();
    }
};
