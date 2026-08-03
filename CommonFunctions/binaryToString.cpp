#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

string readTextFile(const string &fileName)
{
    ifstream file(fileName, ios::binary);

    if (!file.is_open())
    {
        return "";
    }

    return string(istreambuf_iterator<char>(file),
                  istreambuf_iterator<char>());
}

vector<int> strToBinary(const string &text)
{
    vector<int> bits;

    for (unsigned char character : text)
    {
        for (int position = 7; position >= 0; position--)
        {
            bits.push_back((character >> position) & 1);
        }
    }

    return bits;
}

string bitsToString(const vector<int> &bits)
{
    string answer;

    for (size_t index = 0; index < bits.size(); index++)
    {
        answer += bits[index] == 0 ? '0' : '1';

        if ((index + 1) % 8 == 0 && index + 1 < bits.size())
        {
            answer += ' ';
        }
    }

    return answer;
}

void saveBits(const string &fileName, const vector<int> &bits)
{
    filesystem::path path(fileName);

    if (path.has_parent_path())
    {
        filesystem::create_directories(path.parent_path());
    }

    ofstream file(fileName);
    file << bitsToString(bits) << '\n';
}
