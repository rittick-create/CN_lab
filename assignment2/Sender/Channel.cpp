#include <chrono>
#include <random>
#include <string>
#include <thread>

using namespace std;

// Simulates delay, data loss and a single-bit transmission error.
class DataChannel {
private:
    double errorProbability;
    mt19937 randomGenerator;
    uniform_real_distribution<double> probability;

public:
    DataChannel(double selectedProbability)
        : randomGenerator(random_device{}()), probability(0.0, 1.0) {
        errorProbability = selectedProbability;
    }

    // Return false when the frame does not reach the receiver.
    bool Channel(string& frameBits, int timeoutMilliseconds) {
        // A small ordinary propagation delay exists in every transmission.
        int ordinaryDelay = 5 + (randomGenerator() % 16);
        this_thread::sleep_for(chrono::milliseconds(ordinaryDelay));

        if (probability(randomGenerator) >= errorProbability) {
            return true;
        }

        int impairment = randomGenerator() % 3;

        if (impairment == 0) {//drop the frame
            return false;
        }

        if (impairment == 1) {//corrupt
            int position = randomGenerator() % frameBits.length();

            frameBits[position] = frameBits[position] == '0' ? '1' : '0';
            return true;
        }

        // A frame delayed beyond its deadline is treated as lost in transit.
        this_thread::sleep_for(//excessive delay
            chrono::milliseconds(timeoutMilliseconds + 20)
        );
        return false;
    }
};
