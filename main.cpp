#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "CommonFunctions/StringtoBinary.cpp"
#include "Sender/Frame.cpp"
#include "ErrorDetection/Checksum/Checksum.cpp"
#include "ErrorDetection/Checksum/ChecksumDetection.cpp"
#include "ErrorDetection/CRC/CRC.cpp"
#include "ErrorDetection/CRC/CRCDetection.cpp"
#include "Sender/ErrorInjection/errorInjection.cpp"
#include "Receiver/receiver.cpp"

using namespace std;

// Convert the file contents to bits and write them to another file.
bool writeContentBits(vector<string>& fileLines, string outputName) {
    ofstream outputFile(outputName);

    if (!outputFile.is_open()) {
        cerr << "Error: could not create output file: " << outputName << '\n';
        return false;
    }

    int totalLines = fileLines.size();

    for (int i = 0; i < totalLines; i++) {
        outputFile << strToBinary(fileLines[i]);
    }

    return true;
}

// Read the bit file and divide its contents into frames.
bool createFrames(string bitsFileName, vector<Frame>& frames,
                  string errorDetectionType) {
    ifstream bitsFile(bitsFileName);

    if (!bitsFile.is_open()) {
        cerr << "Error: could not open bits file: " << bitsFileName << '\n';
        return false;
    }

    string contentBits;
    bitsFile >> contentBits;
    int totalBits = contentBits.length();

    for (int position = 0; position < totalBits; position += PAYLOAD_SIZE) {
        string payloadBits = contentBits.substr(position, PAYLOAD_SIZE);

        // Every frame receives the same error-detection type.
        frames.push_back(Frame(payloadBits, errorDetectionType));
    }

    return true;
}

// Generate and store a 16-bit checksum in every frame.
void addChecksums(vector<Frame>& frames) {
    int totalFrames = frames.size();

    for (int i = 0; i < totalFrames; i++) {
        string checksum = generateChecksum16(
            frames[i].getHeaderBits(),
            frames[i].getPayloadBits()
        );

        frames[i].storeErrorDetectionBits(checksum);
    }
}

// Generate and store CRC bits in every frame.
bool addCRCs(vector<Frame>& frames, string errorDetectionType) {
    string generator = getCRCGenerator(errorDetectionType);

    if (generator == "") {
        return false;
    }

    int totalFrames = frames.size();

    for (int i = 0; i < totalFrames; i++) {
        string crc = generateCRC(
            frames[i].getHeaderBits(),
            frames[i].getPayloadBits(),
            generator
        );

        if (crc == "") {
            return false;
        }

        frames[i].storeErrorDetectionBits(crc);
    }

    return true;
}

// Apply one selected error-detection method to all frames.
bool applyErrorDetection(vector<Frame>& frames, string errorDetectionType) {
    if (errorDetectionType == "CHECKSUM16") {
        addChecksums(frames);
        return true;
    }

    if (errorDetectionType == "CRC8" ||
        errorDetectionType == "CRC10" ||
        errorDetectionType == "CRC16" ||
        errorDetectionType == "CRC32") {
        return addCRCs(frames, errorDetectionType);
    }

    cerr << "Error: unsupported error-detection type\n";
    return false;
}

// Prepare complete frame copies for transmission.
vector<string> prepareFramesForTransmission(vector<Frame>& frames) {
    vector<string> transmittedFrames;
    int totalFrames = frames.size();

    for (int i = 0; i < totalFrames; i++) {
        transmittedFrames.push_back(frames[i].getCompleteFrameBits());
    }

    return transmittedFrames;
}

// Let the sender select one method for the complete file.
string selectErrorDetectionType() {
    int choice;

    cout << "Select an error-detection method:\n";
    cout << "1. Checksum-16\n";
    cout << "2. CRC-8\n";
    cout << "3. CRC-10\n";
    cout << "4. CRC-16\n";
    cout << "5. CRC-32\n";
    cout << "Enter your choice: ";
    cin >> choice;

    if (choice == 1) {
        return "CHECKSUM16";
    }
    if (choice == 2) {
        return "CRC8";
    }
    if (choice == 3) {
        return "CRC10";
    }
    if (choice == 4) {
        return "CRC16";
    }
    if (choice == 5) {
        return "CRC32";
    }

    return "";
}

// Let the sender select how transmission errors will be injected.
string selectErrorInjectionType() {
    int choice;

    cout << "Select an error-injection method:\n";
    cout << "1. No error\n";
    cout << "2. Single-bit error\n";
    cout << "3. Two isolated single-bit errors\n";
    cout << "4. Odd number of errors\n";
    cout << "5. Burst error\n";
    cout << "Enter your choice: ";
    cin >> choice;

    if (choice == 1) {
        return "NO_ERROR";
    }
    if (choice == 2) {
        return "SINGLE_BIT";
    }
    if (choice == 3) {
        return "TWO_ISOLATED_BITS";
    }
    if (choice == 4) {
        return "ODD_NUMBER_OF_BITS";
    }
    if (choice == 5) {
        return "BURST_ERROR";
    }

    return "";
}

// Print the type, payload, and trailer of every frame.
void printFrames(vector<Frame>& frames) {
    int totalFrames = frames.size();

    for (int i = 0; i < totalFrames; i++) {
        cout << "Frame " << i + 1 << '\n';
        cout << "Type: " << frames[i].getErrorDetectionType() << '\n';
        cout << "Payload: " << frames[i].getPayloadBits() << '\n';
        cout << "Trailer: " << frames[i].getErrorDetectionBits() << '\n';
    }
}

// argc stores the number of command-line arguments.
// argv stores the arguments as strings.
int main(int argc, char* argv[]) {
    // Check whether the input filename was provided.
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    // Open the file in normal text mode.
    ifstream inputFile(argv[1]);

    // Stop if the file cannot be opened.
    if (!inputFile.is_open()) {
        cerr << "Error: could not open input file: " << argv[1] << '\n';
        return 1;
    }

    // Store the file contents one line at a time.
    vector<string> fileLines;
    string line;

    while (getline(inputFile, line)) {
        // getline removes '\n', so add it manually.
        line += '\n';

        fileLines.push_back(line);
    }

    // Convert the file contents and store the result.
    if (!writeContentBits(fileLines, "contentBIts.txt")) {
        return 1;
    }

    // Select one error-detection type for the complete file.
    string errorDetectionType = selectErrorDetectionType();

    if (errorDetectionType == "") {
        cerr << "Error: invalid selection\n";
        return 1;
    }

    // Create frames from the generated bit file.
    vector<Frame> frames;
    if (!createFrames("contentBIts.txt", frames, errorDetectionType)) {
        return 1;
    }

    // Apply the selected type to every frame.
    if (!applyErrorDetection(frames, errorDetectionType)) {
        return 1;
    }

    // Create copies of the complete frames for transmission.
    vector<string> transmittedFrames = prepareFramesForTransmission(frames);

    // Select and apply one error-injection method.
    string injectionType = selectErrorInjectionType();

    if (injectionType == "") {
        cerr << "Error: invalid error-injection selection\n";
        return 1;
    }

    if (!applyErrorInjection(transmittedFrames, injectionType)) {
        cerr << "Error: error injection failed\n";
        return 1;
    }

    // Send the transmitted frame copies to the receiver.
    bool detectionResult = receiveFrames(transmittedFrames);

    cout << "Total frames created: " << frames.size() << '\n';
    cout << "Error detection: " << errorDetectionType << '\n';
    cout << "Error injection: " << injectionType << '\n';

    if (detectionResult == true) {
        cout << "Receiver result: true (no error detected)\n";
    }
    else {
        cout << "Receiver result: false (error detected)\n";
    }

    // printFrames(frames);

    // End the program successfully.
    return 0;
}
