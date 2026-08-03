#include <algorithm>
#include <random>
#include <vector>

enum class ErrorType
{
    OneBit = 1,
    TwoBit = 2,
    Burst = 3,
    OddBits = 4
};

class ErrorInjector
{
public:
    ErrorInjector() : generator(std::random_device{}())
    {
    }

    void inject(std::vector<int> &bits, ErrorType type)
    {
        if (bits.empty())
        {
            return;
        }

        switch (type)
        {
        case ErrorType::OneBit:
            flip(bits, randomPosition(bits.size()));
            break;

        case ErrorType::TwoBit:
            injectTwoBitError(bits);
            break;

        case ErrorType::Burst:
            injectBurstError(bits);
            break;

        case ErrorType::OddBits:
            injectOddBitErrors(bits);
            break;
        }
    }

private:
    std::mt19937 generator;

    size_t randomPosition(size_t bitCount)
    {
        std::uniform_int_distribution<size_t> distribution(0,
                                                            bitCount - 1);
        return distribution(generator);
    }

    static void flip(std::vector<int> &bits, size_t position)
    {
        bits[position] ^= 1;
    }

    void injectTwoBitError(std::vector<int> &bits)
    {
        const size_t first = randomPosition(bits.size());

        if (bits.size() == 1)
        {
            flip(bits, first);
            return;
        }

        size_t second = randomPosition(bits.size());
        while (second == first)
        {
            second = randomPosition(bits.size());
        }

        flip(bits, first);
        flip(bits, second);
    }

    void injectBurstError(std::vector<int> &bits)
    {
        constexpr size_t maximumBurstLength = 4;
        const size_t burstLength =
            std::min(maximumBurstLength, bits.size());
        std::uniform_int_distribution<size_t> distribution(
            0, bits.size() - burstLength);
        const size_t start = distribution(generator);

        for (size_t index = start;
             index < start + burstLength;
             ++index)
        {
            flip(bits, index);
        }
    }

    static void injectOddBitErrors(std::vector<int> &bits)
    {
        for (size_t index = 0; index < bits.size(); index += 2)
        {
            flip(bits, index);
        }
    }
};
