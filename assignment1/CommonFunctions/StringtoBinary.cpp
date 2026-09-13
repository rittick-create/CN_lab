#include <string>

using namespace std;

// Convert every character into its 8-bit binary form.
string strToBinary(string text) {
    string binaryString;
    int textLength = text.length();

    for (int i = 0; i < textLength; i++) {
        unsigned char character = text[i];

        for (int bit = 7; bit >= 0; bit--) {
            if ((character >> bit) & 1) {
                binaryString += '1';
            }
            else {
                binaryString += '0';
            }
        }
    }

    return binaryString;
}
