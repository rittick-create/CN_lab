#include <string>
#include <vector>

using namespace std;

class Receiver
{
public:
    string checkChecksum(const vector<int> &receivedBits)
    {
        Checksum checksum;

        if (checksum.detectError(receivedBits))
        {
            return "REJECTED - Checksum detected an error";
        }

        return "ACCEPTED - No checksum error";
    }

    string checkCRC(const vector<int> &receivedBits, int crcType)
    {
        CRC crc;

        if (crc.detectError(receivedBits, crcType))
        {
            return "REJECTED - CRC detected an error";
        }

        return "ACCEPTED - No CRC error";
    }
};
