#include <string>

using namespace std;

// Convert a non-negative number into a fixed number of bits.
string numberToBits(int number, int totalBits) {
    string bits(totalBits, '0');
    for (int i = totalBits - 1; i >= 0; i--) {
        bits[i] = '0' + number % 2;
        number /= 2;
    }
    return bits;
}

// Each byte is just a number represented by eight bits.
string textToBits(string text) {
    string bits;
    for (unsigned char character : text) {
        bits += numberToBits(character, 8);
    }
    return bits;
}

// Convert a binary string into a non-negative number.
int bitsToNumber(string bits) {
    int number = 0;

    for (char bit : bits) {
        if (bit != '0' && bit != '1') {
            return -1;
        }
        number = number * 2 + (bit - '0');
    }

    return number;
}

// Convert complete groups of eight bits back into bytes.
string bitsToText(string bits) {
    string text = "";

    for (int position = 0; position + 8 <= (int)bits.length(); position += 8) {
        int value = bitsToNumber(bits.substr(position, 8));
        text += (char)value;
    }

    return text;
}
