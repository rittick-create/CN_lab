#include <string>
#include "../CommonFunctions/Binary.cpp"
#include "../ErrorDetection/CRC.cpp"

using namespace std;

const int SOURCE_ADDRESS_BITS = 48;
const int DESTINATION_ADDRESS_BITS = 48;
const int LENGTH_BITS = 16;
const int SEQUENCE_BITS = 8;
const int HEADER_SIZE_BITS = 120;
const int PAYLOAD_SIZE_BYTES = 46;
const int PAYLOAD_SIZE_BITS = PAYLOAD_SIZE_BYTES * 8;
const int FCS_SIZE_BITS = 32;
const int FRAME_SIZE_BITS =
    HEADER_SIZE_BITS + PAYLOAD_SIZE_BITS + FCS_SIZE_BITS;

// Stores the four header fields specified in the assignment.
class Header {
private:
    string sourceAddress;
    string destinationAddress;
    int payloadLength;
    int sequenceNumber;

public:
    Header() {
        sourceAddress = "SRC001";
        destinationAddress = "DST001";
        payloadLength = 0;
        sequenceNumber = 0;
    }

    Header(int length, int sequence) {
        sourceAddress = "SRC001";
        destinationAddress = "DST001";
        payloadLength = length;
        sequenceNumber = sequence;
    }

    string getBits() {
        string bits = "";
        bits += textToBits(sourceAddress);
        bits += textToBits(destinationAddress);
        bits += numberToBits(payloadLength, LENGTH_BITS);
        bits += numberToBits(sequenceNumber, SEQUENCE_BITS);
        return bits;
    }
};

// A data frame contains a 15-byte header, 46-byte payload and 4-byte FCS.
class Frame {
private:
    Header header;
    string payloadBits;
    string fcsBits;
    int payloadLength;
    int sequenceNumber;

public:
    Frame(string payload, int sequence) {
        payloadLength = payload.length();
        sequenceNumber = sequence;
        header = Header(payloadLength, sequenceNumber);
        payloadBits = textToBits(payload);

        while ((int)payloadBits.length() < PAYLOAD_SIZE_BITS) {
            payloadBits += '0';
        }

        fcsBits = generateCRC32(header.getBits() + payloadBits);
    }

    int getSequenceNumber() {
        return sequenceNumber;
    }

    string getCompleteFrameBits() {
        return header.getBits() + payloadBits + fcsBits;
    }
};

// Read the sequence number only after the complete frame passes CRC checking.
int readSequenceNumber(string frameBits) {
    if ((int)frameBits.length() != FRAME_SIZE_BITS) {
        return -1;
    }

    int position = SOURCE_ADDRESS_BITS + DESTINATION_ADDRESS_BITS;
    position += LENGTH_BITS;
    return bitsToNumber(frameBits.substr(position, SEQUENCE_BITS));
}

// Recover the unpadded payload only after CRC checking.
string readPayload(string frameBits) {
    if ((int)frameBits.length() != FRAME_SIZE_BITS) {
        return "";
    }

    int lengthPosition = SOURCE_ADDRESS_BITS + DESTINATION_ADDRESS_BITS;
    int payloadLength = bitsToNumber(
        frameBits.substr(lengthPosition, LENGTH_BITS)
    );

    if (payloadLength < 0 || payloadLength > PAYLOAD_SIZE_BYTES) {
        return "";
    }

    string payloadBits = frameBits.substr(
        HEADER_SIZE_BITS, payloadLength * 8
    );
    return bitsToText(payloadBits);
}
