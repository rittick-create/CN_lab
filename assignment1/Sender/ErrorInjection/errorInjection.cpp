#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

using namespace std;

// Change a 0 to 1 or a 1 to 0.
void flipBit(string& frameBits, int position) {
    if (frameBits[position] == '0') {
        frameBits[position] = '1';
    }
    else {
        frameBits[position] = '0';
    }
}

// Flip one random bit in the protected payload-and-trailer region.
bool injectSingleBitError(string& frameBits) {
    int frameLength = frameBits.length();
    int protectedLength = frameLength - HEADER_SIZE;

    if (protectedLength <= 0) {
        return false;
    }

    int position = HEADER_SIZE + (rand() % protectedLength);
    flipBit(frameBits, position);
    return true;
}

// Flip two bits with one unchanged bit between them.
bool injectTwoIsolatedBitErrors(string& frameBits) {
    int protectedLength = frameBits.length() - HEADER_SIZE;
    int completeWords = protectedLength / 16;

    if (completeWords == 0) {
        return false;
    }

    int wordStart = HEADER_SIZE + (rand() % completeWords) * 16;
    int firstPosition = wordStart + (rand() % 14);
    int secondPosition = firstPosition + 2;

    flipBit(frameBits, firstPosition);
    flipBit(frameBits, secondPosition);
    return true;
}

// Flip three separated bits, which is an odd number of errors.
bool injectOddNumberOfErrors(string& frameBits) {
    int protectedLength = frameBits.length() - HEADER_SIZE;
    int completeWords = protectedLength / 16;

    if (completeWords == 0) {
        return false;
    }

    int wordStart = HEADER_SIZE + (rand() % completeWords) * 16;
    int firstPosition = wordStart + (rand() % 12);

    flipBit(frameBits, firstPosition);
    flipBit(frameBits, firstPosition + 2);
    flipBit(frameBits, firstPosition + 4);
    return true;
}

// Flip eight consecutive bits inside one 16-bit word.
bool injectBurstError(string& frameBits) {
    int protectedLength = frameBits.length() - HEADER_SIZE;
    int completeWords = protectedLength / 16;

    if (completeWords == 0) {
        return false;
    }

    int wordStart = HEADER_SIZE + (rand() % completeWords) * 16;
    int burstStart = wordStart + (rand() % 9);

    for (int i = 0; i < 8; i++) {
        flipBit(frameBits, burstStart + i);
    }

    return true;
}

// Apply one selected injection method to every transmitted frame.
bool applyErrorInjection(vector<string>& transmittedFrames,
                         string injectionType) {
    if (injectionType == "NO_ERROR") {
        return true;
    }

    srand(time(0));
    int totalFrames = transmittedFrames.size();

    if (totalFrames == 0) {
        return false;
    }

    for (int i = 0; i < totalFrames; i++) {
        bool injectionWorked = false;

        if (injectionType == "SINGLE_BIT") {
            injectionWorked = injectSingleBitError(transmittedFrames[i]);
        }
        else if (injectionType == "TWO_ISOLATED_BITS") {
            injectionWorked = injectTwoIsolatedBitErrors(
                transmittedFrames[i]
            );
        }
        else if (injectionType == "ODD_NUMBER_OF_BITS") {
            injectionWorked = injectOddNumberOfErrors(
                transmittedFrames[i]
            );
        }
        else if (injectionType == "BURST_ERROR") {
            injectionWorked = injectBurstError(transmittedFrames[i]);
        }
        else {
            return false;
        }

        if (injectionWorked == false) {
            return false;
        }
    }

    return true;
}
