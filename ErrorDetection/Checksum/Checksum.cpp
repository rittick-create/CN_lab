#include <string>

using namespace std;

// Convert one 16-bit binary word into a number.
int checksumWordToNumber(string word) {
    int number = 0;

    for (int i = 0; i < 16; i++) {
        number = number * 2;

        if (word[i] == '1') {
            number = number + 1;
        }
    }

    return number;
}

// Generate a 16-bit checksum from the payload only.
string generateChecksum16(string payloadBits) {
    // Complete the 384-bit payload with zero padding.
    while (payloadBits.length() < PAYLOAD_SIZE_BITS) {
        payloadBits += '0';
    }

    if (payloadBits.length() > PAYLOAD_SIZE_BITS) {
        payloadBits = payloadBits.substr(0, PAYLOAD_SIZE_BITS);
    }

    string dataword = payloadBits;

    // Make the dataword length divisible by 16.
    while (dataword.length() % 16 != 0) {
        dataword += '0';
    }

    int sum = 0;
    int datawordLength = dataword.length();

    // Add all the 16-bit words.
    for (int position = 0; position < datawordLength; position += 16) {
        string word = dataword.substr(position, 16);
        sum = sum + checksumWordToNumber(word);

        // Wrap the carry around to the right side.
        if (sum > 65535) {
            sum = (sum - 65536) + 1;
        }
    }

    // Invert the final 16-bit sum.
    sum = 65535 - sum;

    string checksum = "";

    // Convert the checksum number into 16 bits.
    for (int i = 0; i < 16; i++) {
        if (sum % 2 == 0) {
            checksum = '0' + checksum;
        }
        else {
            checksum = '1' + checksum;
        }

        sum = sum / 2;
    }

    return checksum;
}
