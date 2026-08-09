#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "CommonFunctions/StringtoBinary.cpp"
#include "Sender/Frame.cpp"

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
bool createFrames(string bitsFileName, vector<Frame>& frames) {
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

        frames.push_back(Frame(payloadBits));
    }

    return true;
}

// Print the payload bits stored in every frame.
void printFrames(vector<Frame>& frames) {
    int totalFrames = frames.size();

    for (int i = 0; i < totalFrames; i++) {
        cout << "Frame " << i + 1 << ": ";
        cout << frames[i].getPayloadBits() << '\n';
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

    // Create frames from the generated bit file.
    vector<Frame> frames;
    if (!createFrames("contentBIts.txt", frames)) {
        return 1;
    }

    cout << "Total frames created: " << frames.size() << '\n';
    // printFrames(frames);

    // End the program successfully.
    return 0;
}
