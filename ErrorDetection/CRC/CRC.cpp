#include <string>

using namespace std;

// Return the generator polynomial for the selected CRC type.
string getCRCGenerator(string errorDetectionType) {
    if (errorDetectionType == "CRC8") {
        return "100000111";
    }

    if (errorDetectionType == "CRC10") {
        return "11000110011";
    }

    if (errorDetectionType == "CRC16") {
        return "11000000000000101";
    }

    if (errorDetectionType == "CRC32") {
        return "100000100110000010001110110110111";
    }

    return "";
}

// Generate CRC bits using modulo-2 division.
string generateCRC(string headerBits, string payloadBits, string generator) {
    // Complete the payload before calculating the CRC.
    while (payloadBits.length() < 48) {
        payloadBits += '0';
    }

    if (payloadBits.length() > 48) {
        payloadBits = payloadBits.substr(0, 48);
    }

    int generatorLength = generator.length();

    if (generatorLength < 2) {
        return "";
    }

    int crcLength = generatorLength - 1;
    string dividend = headerBits + payloadBits;

    // Append zeros equal to the required CRC length.
    for (int i = 0; i < crcLength; i++) {
        dividend += '0';
    }

    int dividendLength = dividend.length();

    // Perform binary division using XOR.
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

    // The final bits are the CRC remainder.
    return dividend.substr(dividendLength - crcLength, crcLength);
}
