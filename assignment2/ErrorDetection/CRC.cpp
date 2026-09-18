#include <string>

using namespace std;

// The same classroom CRC-32 generator used in Assignment 1.
const string CRC32_GENERATOR =
    "100000100110000010001110110110111";

// Return the remainder produced by modulo-2 division.
string divideByGenerator(string dividend, string generator) {
    int generatorLength = generator.length();
    int dividendLength = dividend.length();

    for (int position = 0;
         position <= dividendLength - generatorLength;
         position++) {
        if (dividend[position] == '1') {
            for (int i = 0; i < generatorLength; i++) {
                // XOR: equal bits give 0; different bits give 1.
                dividend[position + i] =
                    dividend[position + i] == generator[i] ? '0' : '1';
            }
        }
    }

    return dividend.substr(dividendLength - generatorLength + 1);
}

// Generate the 32-bit FCS for a frame's header and payload.
string generateCRC32(string protectedBits) {
    return divideByGenerator(protectedBits + string(32, '0'), CRC32_GENERATOR);
}

// A correct codeword gives an all-zero remainder.
bool detectCRC32(string codeword) {
    return divideByGenerator(codeword, CRC32_GENERATOR) == string(32, '0');
}
