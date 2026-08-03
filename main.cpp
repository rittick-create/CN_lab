#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include "./CommonFunctions/binaryToString.cpp"
#include "./Sender/ErrorInjection/errorInjection.cpp"
#include "./Sender/Frame.cpp"

using namespace std;

int main()
{
    //Take the input from the text file
    ifstream file("textfile.txt");

    if (!file.is_open())
    {
        cout << "Could not open the file\n";
        return 1;
    }

    vector<string> fileData;
    string line;

    while (getline(file, line))
    {
        fileData.push_back(line);
    }

    file.close();

    const filesystem::path outputDirectory =
        "Sender/Input file(Bits)";
    filesystem::create_directories(outputDirectory);

    const filesystem::path bitsPath =
        outputDirectory / "input_bits.txt";
    filesystem::remove(bitsPath);

    ofstream bitsFile(bitsPath, ios::trunc);
    if (!bitsFile.is_open())
    {
        cerr << "Could not create the bits file\n";
        return 1;
    }

    vector<string> bitData;
    bitData.reserve(fileData.size());
    size_t totalBits = 0;

    for (size_t index = 0; index < fileData.size(); ++index)
    {
        const vector<int> bits =
            strToBinary(fileData[index]);
        bitData.emplace_back();
        string &bitString = bitData.back();
        bitString.reserve(bits.size());

        for (int bit : bits)
        {
            const char bitCharacter = bit == 0 ? '0' : '1';
            bitString.push_back(bitCharacter);
            bitsFile.put(bitCharacter);
        }

        totalBits += bits.size();

        if (index + 1 < fileData.size())
        {
            bitsFile.put('\n');
        }
    }

    vector<Framing> frames;
    frames.reserve((totalBits + Framing::PAYLOAD_LENGTH - 1) /
                   Framing::PAYLOAD_LENGTH);

    size_t dataIndex = 0;
    size_t bitIndex = 0;

    while (dataIndex < bitData.size())
    {
        Framing frame;
        if (!frame.createFrame(bitData, dataIndex, bitIndex))
        {
            break;
        }

        frame.header.frameNumber =
            static_cast<int>(frames.size() + 1);
        frames.push_back(std::move(frame));
    }

    cout << "Choose error injection type:\n"
         << "1. One-bit error\n"
         << "2. Two-bit error\n"
         << "3. Burst error\n"
         << "4. Odd-position bits error\n"
         << "Enter option: ";

    int errorChoice = 0;
    if (!(cin >> errorChoice) || errorChoice < 1 || errorChoice > 4)
    {
        cerr << "Invalid error injection option\n";
        return 1;
    }

    ErrorInjector errorInjector;
    const ErrorType errorType = static_cast<ErrorType>(errorChoice);

    for (Framing &frame : frames)
    {
        errorInjector.inject(frame.payload.bits, errorType);
    }

    const filesystem::path framesDirectory = "Sender/Frames";
    filesystem::create_directories(framesDirectory);

    for (const filesystem::directory_entry &entry :
         filesystem::directory_iterator(framesDirectory))
    {
        const string fileName = entry.path().filename().string();
        if (entry.is_regular_file() &&
            fileName.rfind("frame_", 0) == 0 &&
            entry.path().extension() == ".txt")
        {
            filesystem::remove(entry.path());
        }
    }

    for (size_t index = 0; index < frames.size(); ++index)
    {
        const filesystem::path framePath =
            framesDirectory /
            ("frame_" + to_string(index + 1) + ".txt");
        ofstream frameFile(framePath, ios::trunc);

        if (!frameFile.is_open())
        {
            cerr << "Could not create " << framePath << '\n';
            return 1;
        }

        for (int bit : frames[index].payload.bits)
        {
            frameFile.put(bit == 0 ? '0' : '1');
        }
    }

    return 0;
}
