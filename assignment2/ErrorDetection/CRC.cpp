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
                if (dividend[position + i] == generator[i]) {
                    dividend[position + i] = '0';
                }
                else {
                    dividend[position + i] = '1';
                }
            }
        }
    }

    return dividend.substr(dividendLength - generatorLength + 1);
}

// Generate the 32-bit FCS for a frame's header and payload.
string generateCRC32(string protectedBits) {
    string dividend = protectedBits;

    for (int i = 0; i < 32; i++) {
        dividend += '0';
    }

    return divideByGenerator(dividend, CRC32_GENERATOR);
}

// A correct codeword gives an all-zero remainder.
bool detectCRC32(string codeword) {
    string remainder = divideByGenerator(codeword, CRC32_GENERATOR);

    for (int i = 0; i < (int)remainder.length(); i++) {
        if (remainder[i] != '0') {
            return false;
        }
    }

    return true;
}
