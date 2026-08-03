#include <string>
#include <vector>

using namespace std;

class Checksum
{
private:
    unsigned int addWords(const vector<int> &bits)
    {
        unsigned int sum = 0;

        for (size_t start = 0; start < bits.size(); start += 16)
        {
            unsigned int word = 0;

            for (int index = 0; index < 16; index++)
            {
                word = word << 1;

                if (start + index < bits.size())
                {
                    word = word + bits[start + index];
                }
            }

            sum = sum + word;

            while (sum > 65535)
            {
                sum = (sum & 65535) + (sum >> 16);
            }
        }

        return sum;
    }

public:
    vector<int> generate(const vector<int> &dataBits)
    {
        unsigned int value = (~addWords(dataBits)) & 65535;
        vector<int> checksum;

        for (int position = 15; position >= 0; position--)
        {
            checksum.push_back((value >> position) & 1);
        }

        return checksum;
    }

    vector<int> createCodeword(const vector<int> &dataBits)
    {
        vector<int> codeword = dataBits;
        vector<int> checksum = generate(dataBits);
        codeword.insert(codeword.end(),
                        checksum.begin(), checksum.end());
        return codeword;
    }

    bool detectError(const vector<int> &receivedBits)
    {
        return addWords(receivedBits) != 65535;
    }

    string checksumText(const vector<int> &dataBits)
    {
        vector<int> value = generate(dataBits);
        string text;

        for (int bit : value)
        {
            text += bit == 0 ? '0' : '1';
        }

        return text;
    }
};
