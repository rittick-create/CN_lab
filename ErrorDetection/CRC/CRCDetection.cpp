#include <string>

using namespace std;

// Return true when the received CRC remainder is zero.
bool detectCRC(string payloadBits, string receivedCRC, string generator) {
    int generatorLength = generator.length();

    if (generatorLength < 2) {
        return false;
    }

    int crcLength = generatorLength - 1;
    int receivedCRCLength = receivedCRC.length();

    if (payloadBits.length() != PAYLOAD_SIZE_BITS ||
        receivedCRCLength != crcLength) {
        return false;
    }

    string receivedData = payloadBits + receivedCRC;
    int receivedLength = receivedData.length();

    // Reject anything other than binary digits.
    for (int i = 0; i < receivedLength; i++) {
        if (receivedData[i] != '0' && receivedData[i] != '1') {
            return false;
        }
    }

    // Divide the received payload-and-CRC codeword using XOR.
    for (int position = 0;
         position <= receivedLength - generatorLength;
         position++) {
        if (receivedData[position] == '1') {
            for (int i = 0; i < generatorLength; i++) {
                if (receivedData[position + i] == generator[i]) {
                    receivedData[position + i] = '0';
                }
                else {
                    receivedData[position + i] = '1';
                }
            }
        }
    }

    // A valid frame has an all-zero remainder.
    for (int i = receivedLength - crcLength; i < receivedLength; i++) {
        if (receivedData[i] != '0') {
            return false;
        }
    }

    return true;
}
