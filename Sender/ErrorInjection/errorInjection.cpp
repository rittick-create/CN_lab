#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace std;

enum ErrorType
{
    NO_ERROR = 0,
    SINGLE_BIT = 1,
    TWO_BITS = 2,
    ODD_BITS = 3,
    BURST_ERROR = 4
};

class ErrorInjector
{
private:
    void flip(vector<int> &bits, int position)
    {
        bits[position] = bits[position] == 0 ? 1 : 0;
    }

    bool alreadyChosen(const vector<int> &positions, int value)
    {
        return find(positions.begin(), positions.end(), value) !=
               positions.end();
    }

public:
    vector<int> inject(vector<int> &bits,
                       int errorType,
                       int payloadLength)
    {
        vector<int> positions;

        if (errorType == NO_ERROR || payloadLength == 0)
        {
            return positions;
        }

        if (errorType == SINGLE_BIT)
        {
            positions.push_back(rand() % payloadLength);
        }
        else if (errorType == TWO_BITS)
        {
            int first = rand() % payloadLength;
            int second = rand() % payloadLength;

            while (second == first || abs(second - first) == 1)
            {
                second = rand() % payloadLength;
            }

            positions.push_back(first);
            positions.push_back(second);
        }
        else if (errorType == ODD_BITS)
        {
            while (positions.size() < 3)
            {
                int value = rand() % payloadLength;

                if (!alreadyChosen(positions, value))
                {
                    positions.push_back(value);
                }
            }
        }
        else if (errorType == BURST_ERROR)
        {
            int burstLength = 4;
            int start = rand() % (payloadLength - burstLength + 1);

            for (int index = 0; index < burstLength; index++)
            {
                positions.push_back(start + index);
            }
        }

        sort(positions.begin(), positions.end());

        for (int position : positions)
        {
            flip(bits, position);
        }

        return positions;
    }
};
