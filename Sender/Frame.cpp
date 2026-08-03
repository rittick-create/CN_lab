#include <string>
#include <vector>

using namespace std;

class Header
{
public:
    string sourceMac;
    string destinationMac;
    int frameNumber;
    int payloadLength;

    Header()
    {
        sourceMac = "";
        destinationMac = "";
        frameNumber = 0;
        payloadLength = 0;
    }
};

class Payload
{
public:
    vector<int> bits;
};

class Trailer
{
public:
    string errorBits;
};

class Framing
{
public:
    static constexpr size_t PAYLOAD_LENGTH = 48;

    Header header;
    Payload payload;
    Trailer trailer;

    bool createFrame(const vector<string> &bitData,
                     size_t &dataIndex,
                     size_t &bitIndex)
    {
        payload.bits.clear();
        payload.bits.reserve(PAYLOAD_LENGTH);

        while (dataIndex < bitData.size() &&
               payload.bits.size() < PAYLOAD_LENGTH)
        {
            const string &part = bitData[dataIndex];

            while (bitIndex < part.size() &&
                   payload.bits.size() < PAYLOAD_LENGTH)
            {
                payload.bits.push_back(part[bitIndex] == '1' ? 1 : 0);
                ++bitIndex;
            }

            if (bitIndex == part.size())
            {
                ++dataIndex;
                bitIndex = 0;
            }
        }

        if (payload.bits.empty())
        {
            return false;
        }

        payload.bits.resize(PAYLOAD_LENGTH, 0);
        header.payloadLength = static_cast<int>(PAYLOAD_LENGTH);
        return true;
    }
};
