#include <string>
#include <vector>

using namespace std;

enum CRCType
{
    CRC8 = 1,
    CRC10 = 2,
    CRC16 = 3,
    CRC32 = 4
};

class CRC
{
private:
    vector<int> textToBits(const string &text)
    {
        vector<int> bits;

        for (char value : text)
        {
            bits.push_back(value == '0' ? 0 : 1);
        }

        return bits;
    }

    vector<int> divide(vector<int> work,
                       const vector<int> &polynomial)
    {
        int lastStart = work.size() - polynomial.size();

        for (int start = 0; start <= lastStart; start++)
        {
            if (work[start] == 1)
            {
                for (size_t index = 0;
                     index < polynomial.size(); index++)
                {
                    work[start + index] =
                        work[start + index] ^ polynomial[index];
                }
            }
        }

        return work;
    }

public:
    vector<int> getPolynomial(int type)
    {
        if (type == CRC8)
        {
            return textToBits("111010101");
        }
        if (type == CRC10)
        {
            return textToBits("11000110011");
        }
        if (type == CRC16)
        {
            return textToBits("11000000000000101");
        }
        return textToBits("100000100110000010001110110110111");
    }

    string name(int type)
    {
        if (type == CRC8)
            return "CRC-8";
        if (type == CRC10)
            return "CRC-10";
        if (type == CRC16)
            return "CRC-16";
        return "CRC-32";
    }

    vector<int> generate(const vector<int> &dataBits, int type)
    {
        vector<int> polynomial = getPolynomial(type);
        int degree = polynomial.size() - 1;
        vector<int> work = dataBits;

        for (int count = 0; count < degree; count++)
        {
            work.push_back(0);
        }

        work = divide(work, polynomial);
        return vector<int>(work.end() - degree, work.end());
    }

    vector<int> createCodeword(const vector<int> &dataBits, int type)
    {
        vector<int> codeword = dataBits;
        vector<int> remainder = generate(dataBits, type);
        codeword.insert(codeword.end(),
                        remainder.begin(), remainder.end());
        return codeword;
    }

    bool detectError(const vector<int> &receivedBits, int type)
    {
        vector<int> polynomial = getPolynomial(type);
        vector<int> result = divide(receivedBits, polynomial);
        int degree = polynomial.size() - 1;

        for (int index = result.size() - degree;
             index < static_cast<int>(result.size()); index++)
        {
            if (result[index] != 0)
            {
                return true;
            }
        }

        return false;
    }

    string remainderText(const vector<int> &dataBits, int type)
    {
        vector<int> remainder = generate(dataBits, type);
        string text;

        for (int bit : remainder)
        {
            text += bit == 0 ? '0' : '1';
        }

        return text;
    }
};
