#include <string>
#include <vector>

inline std::vector<int> strToBinary(const std::string &text)
{
    std::vector<int> bits;
    bits.reserve(text.size() * 8);

    for (unsigned char character : text)
    {
        for (int bitPosition = 7; bitPosition >= 0; --bitPosition)
        {
            bits.push_back((character >> bitPosition) & 1);
        }
    }

    return bits;
}
