#include <vector>

using namespace std;

class Frame
{
public:
    int frameNumber;
    int originalBitCount;
    vector<int> payload;
};

class Framing
{
public:
    static const int PAYLOAD_LENGTH = 46 * 8;

    vector<Frame> createFrames(const vector<int> &allBits)
    {
        vector<Frame> frames;
        int current = 0;
        int number = 1;

        while (current < static_cast<int>(allBits.size()))
        {
            Frame frame;
            frame.frameNumber = number;
            frame.originalBitCount = 0;

            while (current < static_cast<int>(allBits.size()) &&
                   frame.payload.size() < PAYLOAD_LENGTH)
            {
                frame.payload.push_back(allBits[current]);
                current++;
                frame.originalBitCount++;
            }

            while (frame.payload.size() < PAYLOAD_LENGTH)
            {
                frame.payload.push_back(0);
            }

            frames.push_back(frame);
            number++;
        }

        return frames;
    }
};
