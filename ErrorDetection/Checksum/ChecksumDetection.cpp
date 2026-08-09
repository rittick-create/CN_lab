#include <string>

using namespace std;

// Convert one received 16-bit word into a number.
int receivedWordToNumber(string word) {
    int number = 0;

    for (int i = 0; i < 16; i++) {
        number = number * 2;

        if (word[i] == '1') {
            number = number + 1;
        }
    }

    return number;
}

// Return true when the received checksum is valid.
bool detectChecksum16(string headerBits, string payloadBits,
                      string receivedChecksum) {
    if (receivedChecksum.length() != 16) {
        return false;
    }

    // Complete the 48-bit payload with zero padding.
    while (payloadBits.length() < 48) {
        payloadBits += '0';
    }

    if (payloadBits.length() > 48) {
        payloadBits = payloadBits.substr(0, 48);
    }

    string receivedData = headerBits + payloadBits;

    // Use the same temporary padding used during generation.
    while (receivedData.length() % 16 != 0) {
        receivedData += '0';
    }

    // The receiver also adds the received checksum.
    receivedData += receivedChecksum;

    int sum = 0;
    int receivedDataLength = receivedData.length();

    for (int position = 0; position < receivedDataLength; position += 16) {
        string word = receivedData.substr(position, 16);

        // Reject data containing anything other than 0 or 1.
        for (int i = 0; i < 16; i++) {
            if (word[i] != '0' && word[i] != '1') {
                return false;
            }
        }

        sum = sum + receivedWordToNumber(word);

        // Wrap the carry around to the right side.
        if (sum > 65535) {
            sum = (sum - 65536) + 1;
        }
    }

    // A correct frame produces sixteen 1s.
    return sum == 65535;
}
