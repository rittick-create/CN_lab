#include "cdma.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace std;
namespace fs = std::filesystem;

int main() {
    try {
        fs::create_directories("sender");
        fs::create_directories("receiver");
        int n;
        cout << "Number of stations: ";
        if (!(cin >> n) || n < 1)
            throw runtime_error("Enter a positive station count.");
        auto codes = walsh(n);
        vector<string> names(n);
        vector<ifstream> inputs(n);
        vector<ofstream> outputs(n);

        for (int i = 0; i < n; ++i) {
            names[i] = "sender" + to_string(i + 1) + ".txt";
            cout << "Station " << i + 1 << ": sender/" << names[i] << '\n';
            inputs[i].open(fs::path("sender") / names[i], ios::binary);
            if (!inputs[i]) throw runtime_error("Cannot read sender/" + names[i]);
        }
        for (int i = 0; i < n; ++i) {
            outputs[i].open(fs::path("receiver") / names[i], ios::binary);
            if (!outputs[i]) throw runtime_error("Cannot write receiver/" + names[i]);
        }

        // Process one byte per station at a time, transmitting its bits MSB first.
        while (true) {
            vector<int> bytes(n), recovered(n, 0), active(n, 0);
            bool any = false;
            for (int i = 0; i < n; ++i) {
                char byte;
                if (inputs[i].get(byte)) {
                    bytes[i] = static_cast<unsigned char>(byte);
                    active[i] = 1;
                    any = true;
                } else if (!inputs[i].eof()) {
                    throw runtime_error("Read failed for " + names[i]);
                }
            }
            if (!any) break;

            for (int bit = 7; bit >= 0; --bit) {
                vector<int> data(n, 0); // Finished stations remain silent.
                for (int i = 0; i < n; ++i)
                    if (active[i]) data[i] = (bytes[i] >> bit & 1) ? 1 : -1;
                auto signal = transmit(data, codes);
                auto decoded = receive(signal, codes);
                for (int i = 0; i < n; ++i)
                    if (decoded[i] == 1) recovered[i] |= 1 << bit;
            }
            for (int i = 0; i < n; ++i)
                if (active[i]) outputs[i].put(static_cast<char>(recovered[i]));
        }
        for (int i = 0; i < n; ++i) {
            outputs[i].close();
            if (!outputs[i]) throw runtime_error("Write failed for " + names[i]);
            cout << "Received: receiver/" << names[i] << '\n';
        }
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}
