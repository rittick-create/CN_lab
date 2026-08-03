#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "CommonFunctions/binaryToString.cpp"
#include "Sender/Frame.cpp"
#include "Sender/ErrorInjection/errorInjection.cpp"
#include "ErrorDetection/Checksum/Checksum.cpp"
#include "ErrorDetection/CRC/CRC.cpp"
#include "Receiver/receiver.cpp"
#include "Evaluation/evaluation.cpp"
using namespace std;
string errorName(int choice)
{
    if (choice == 0)
        return "No error";
    if (choice == 1)
        return "Single-bit error";
    if (choice == 2)
        return "Two isolated bit errors";
    if (choice == 3)
        return "Odd number of errors";
    return "Burst error";
}
string positionsText(const vector<int> &positions)
{
    if (positions.empty())
        return "None";
    string text;
    for (size_t index = 0; index < positions.size(); index++)
    {
        if (index > 0)
            text += ", ";
        text += to_string(positions[index]);
    }
    return text;
}
void showMenu()
{
    cout << "\nChoose an operation:\n"
         << "1. Checksum-16\n"
         << "2. CRC\n"
         << "3. Evaluate all schemes\n"
         << "Enter choice: ";
}
void showErrorMenu()
{
    cout << "\nChoose error type:\n"
         << "0. No error\n"
         << "1. Single-bit error\n"
         << "2. Two isolated bit errors\n"
         << "3. Odd number of errors\n"
         << "4. Burst error\n"
         << "Enter choice: ";
}
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: ./cn_lab <input-file>\n"
             << "Example: ./cn_lab textfile.txt\n";
        return 1;
    }
    string inputFile = argv[1];
    if (!filesystem::exists(inputFile))
    {
        cout << "Could not find " << inputFile << '\n';
        return 1;
    }
    string fileText = readTextFile(inputFile);
    if (fileText.empty())
    {
        cout << "The input file is empty.\n";
        return 1;
    }
    vector<int> allBits = strToBinary(fileText);
    saveBits("Sender/Input file(Bits)/input_bits.txt", allBits);
    Framing framing;
    vector<Frame> frames = framing.createFrames(allBits);
    srand(static_cast<unsigned int>(time(0)));
    cout << "\nInput text  : " << fileText << '\n'
         << "Input bits  : " << allBits.size() << '\n'
         << "Frame size  : 368 payload bits (46 bytes)\n"
         << "Total frames: " << frames.size() << '\n';
    showMenu();
    int operation;
    cin >> operation;
    if (operation == 3)
    {
        Evaluation evaluation;
        evaluation.run(frames[0]);
        return 0;
    }
    if (operation != 1 && operation != 2)
    {
        cout << "Invalid operation.\n";
        return 1;
    }
    int crcType = CRC8;
    if (operation == 2)
    {
        cout << "\nChoose CRC: 1.CRC-8  2.CRC-10  "
             << "3.CRC-16  4.CRC-32\nEnter choice: ";
        cin >> crcType;
        if (crcType < 1 || crcType > 4)
        {
            cout << "Invalid CRC choice.\n";
            return 1;
        }
    }
    showErrorMenu();
    int errorChoice;
    cin >> errorChoice;
    if (errorChoice < 0 || errorChoice > 4)
    {
        cout << "Invalid error choice.\n";
        return 1;
    }
    ErrorInjector injector;
    Receiver receiver;
    Checksum checksum;
    CRC crc;
    ostringstream resultFile;
    resultFile << "Input file: " << inputFile << '\n'
               << "Frames: " << frames.size() << '\n'
               << "Error: " << errorName(errorChoice) << "\n\n";
    for (Frame frame : frames)
    {
        vector<int> cleanCodeword;
        string checkBits;
        if (operation == 1)
        {
            cleanCodeword = checksum.createCodeword(frame.payload);
            checkBits = checksum.checksumText(frame.payload);
        }
        else
        {
            cleanCodeword = crc.createCodeword(frame.payload, crcType);
            checkBits = crc.remainderText(frame.payload, crcType);
        }
        vector<int> sentCodeword = cleanCodeword;
        vector<int> changed = injector.inject(
            sentCodeword, errorChoice, frame.payload.size());
        string receiverResult = operation == 1
                                    ? receiver.checkChecksum(sentCodeword)
                                    : receiver.checkCRC(sentCodeword, crcType);
        string number = to_string(frame.frameNumber);
        saveBits("Sender/Frames/clean_frame_" + number + ".txt",
                 cleanCodeword);
        saveBits("Sender/Frames/sent_frame_" + number + ".txt",
                 sentCodeword);
        cout << "\nFrame " << frame.frameNumber << '\n'
             << "Check bits       : " << checkBits << '\n'
             << "Changed positions: " << positionsText(changed) << '\n'
             << "Receiver result  : " << receiverResult << '\n';
        resultFile << "Frame " << frame.frameNumber << '\n'
                   << "Check bits: " << checkBits << '\n'
                   << "Changed positions: " << positionsText(changed) << '\n'
                   << "Result: " << receiverResult << "\n\n";
    }
    filesystem::create_directories("Receiver");
    ofstream receiverOutput("Receiver/result.txt");
    receiverOutput << resultFile.str();
    cout << "\nClean and sent frames are in Sender/Frames.\n"
         << "Receiver result is in Receiver/result.txt.\n";
    return 0;
}
