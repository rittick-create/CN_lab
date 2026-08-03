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
    Header header;
    Payload payload;
    Trailer trailer;

    // Convert one string into bits
    vector<int> convertToBits(const string &text)
    {
        vector<int> bits;
        bits.reserve(text.size() * 8);

        for (int i = 0; i < (int)text.size(); i++)
        {
            unsigned char character =
                static_cast<unsigned char>(text[i]);

            // Extract bits from MSB to LSB
            for (int bitPosition = 7; bitPosition >= 0; bitPosition--)
            {
                int bit = (character >> bitPosition) & 1;
                bits.push_back(bit);
            }
        }

        return bits;
    }

    void createFrame(const vector<string> &fileData)
    {
        constexpr size_t payloadLength = 48;
        string frameData;

        frameData.reserve(payloadLength);

        for (const string &part : fileData)
        {
            const size_t remaining = payloadLength - frameData.size();
            if (remaining == 0)
            {
                break;
            }

            const size_t bytesToCopy =
                part.size() < remaining ? part.size() : remaining;
            frameData.append(part, 0, bytesToCopy);
        }

        frameData.resize(payloadLength, '\0');
        header.payloadLength = static_cast<int>(payloadLength);

        payload.bits = convertToBits(frameData);
    }
};
