#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <utility>
#include <vector>
using namespace std;
struct ResultRow
{
    string scheme;
    string error;
    int detected;
    double time;
};
class Evaluation
{
private:
    vector<int> makeCodeword(const vector<int> &data, int scheme)
    {
        if (scheme == 0)
        {
            Checksum checksum;
            return checksum.createCodeword(data);
        }
        CRC crc;
        return crc.createCodeword(data, scheme);
    }
    bool errorDetected(const vector<int> &word, int scheme)
    {
        if (scheme == 0)
        {
            Checksum checksum;
            return checksum.detectError(word);
        }
        CRC crc;
        return crc.detectError(word, scheme);
    }
    string schemeName(int scheme)
    {
        if (scheme == 0)
            return "Checksum-16";
        CRC crc;
        return crc.name(scheme);
    }
    string errorName(int error)
    {
        if (error == 1)
            return "Single";
        if (error == 2)
            return "Two bits";
        if (error == 3)
            return "Odd bits";
        return "Burst";
    }
    void flipPositions(vector<int> &bits,
                       const vector<int> &positions)
    {
        for (int position : positions)
        {
            bits[position] = bits[position] == 0 ? 1 : 0;
        }
    }
    pair<bool, bool> testPattern(const vector<int> &data,
                                 int crcType,
                                 const vector<int> &positions)
    {
        Checksum checksum;
        CRC crc;
        vector<int> checksumWord = checksum.createCodeword(data);
        vector<int> crcWord = crc.createCodeword(data, crcType);
        flipPositions(checksumWord, positions);
        flipPositions(crcWord, positions);
        return {checksum.detectError(checksumWord),
                crc.detectError(crcWord, crcType)};
    }
public:
    void run(const Frame &frame)
    {
        const int trials = 100;
        vector<ResultRow> rows;
        ErrorInjector injector;
        for (int scheme = 0; scheme <= 4; scheme++)
        {
            for (int error = 1; error <= 4; error++)
            {
                int detected = 0;
                auto start = chrono::high_resolution_clock::now();
                for (int trial = 0; trial < trials; trial++)
                {
                    vector<int> word =
                        makeCodeword(frame.payload, scheme);
                    injector.inject(word, error, frame.payload.size());
                    if (errorDetected(word, scheme))
                    {
                        detected++;
                    }
                }
                auto stop = chrono::high_resolution_clock::now();
                double microseconds =
                    chrono::duration<double, micro>(stop - start).count();
                rows.push_back({schemeName(scheme), errorName(error),
                                detected, microseconds / trials});
            }
        }
        ostringstream report;
        report << "EVALUATION TABLE (100 trials)\n\n"
               << left << setw(13) << "Scheme" << setw(11) << "Error"
               << setw(11) << "Detected" << "Avg time (us)\n"
               << string(50, '-') << '\n' << fixed << setprecision(3);
        for (ResultRow row : rows)
        {
            report << left << setw(13) << row.scheme
                   << setw(11) << row.error
                   << setw(11) << (to_string(row.detected) + "/100")
                   << row.time << '\n';
        }
        report << "\nASCII GRAPH (# = 5% detection)\n\n";
        for (ResultRow row : rows)
        {
            report << setw(13) << row.scheme << setw(11) << row.error << " ";
            for (int count = 0; count < row.detected / 5; count++)
            {
                report << '#';
            }
            report << " " << row.detected << "%\n";
        }
        vector<int> sampleData;
        for (int value = 0; value < 46; value++)
        {
            for (int bit = 7; bit >= 0; bit--)
            {
                sampleData.push_back((value >> bit) & 1);
            }
        }
        report << "\nSPECIAL COMPARISON CASES\n"
               << "D = detected, M = missed\n\n"
               << setw(9) << "CRC" << setw(18) << "Case"
               << "Checksum  CRC\n" << string(45, '-') << '\n';
        CRC crc;
        for (int type = 1; type <= 4; type++)
        {
            vector<int> generatorError;
            vector<int> polynomial = crc.getPolynomial(type);
            for (size_t index = 0; index < polynomial.size(); index++)
            {
                if (polynomial[index] == 1)
                    generatorError.push_back(index);
            }
            vector<pair<string, vector<int>>> cases = {
                {"Both detect", {0}},
                {"CRC only", {6, 22}},
                {"Checksum only", generatorError}};
            for (auto item : cases)
            {
                pair<bool, bool> answer =
                    testPattern(sampleData, type, item.second);
                report << setw(9) << crc.name(type)
                       << setw(18) << item.first
                       << setw(10) << (answer.first ? "D" : "M")
                       << (answer.second ? "D" : "M") << '\n';
            }
        }
        report << "\nRESOURCE USE (extra bits per frame)\n"
               << "Checksum-16: 16\nCRC-8: 8\nCRC-10: 10\n"
               << "CRC-16: 16\nCRC-32: 32\n";
        filesystem::create_directories("Results");
        ofstream file("Results/evaluation.txt");
        file << report.str();
        cout << report.str();
        cout << "\nSaved in Results/evaluation.txt\n";
    }
};
