#include <string>

using namespace std;

// Convert every byte in a string into eight binary digits.
string textToBits(string text) {
    string bits = "";

    for (int i = 0; i < (int)text.length(); i++) {
        unsigned char character = text[i];

        for (int bit = 7; bit >= 0; bit--) {
            if ((character >> bit) & 1) {
                bits += '1';
            }
            else {
                bits += '0';
            }
        }
    }

    return bits;
}

// Convert a non-negative number into a fixed number of bits.
string numberToBits(int number, int totalBits) {
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

// Convert a binary string into a non-negative number.
int bitsToNumber(string bits) {
    int number = 0;

    for (int i = 0; i < (int)bits.length(); i++) {
        number = number * 2;

        if (bits[i] == '1') {
            number++;
        }
        else if (bits[i] != '0') {
            return -1;
        }
    }

    return number;
}

// Convert complete groups of eight bits back into bytes.
string bitsToText(string bits) {
    string text = "";

    for (int position = 0;
         position + 8 <= (int)bits.length();
         position += 8) {
        int value = bitsToNumber(bits.substr(position, 8));
        text += (char)value;
    }

    return text;
}
