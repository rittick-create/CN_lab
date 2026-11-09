#include "cdma.h"
#include <stdexcept>
using namespace std;

vector<int> receive(const vector<int>& signal, const vector<vector<int>>& codes) {
    if (signal.empty() || codes.empty())
        throw invalid_argument("Signal and codes must not be empty.");
    vector<int> data(codes.size(), 0);
    for (size_t i = 0; i < codes.size(); ++i) {
        if (codes[i].size() != signal.size())
            throw invalid_argument("Code length must match signal length.");
        for (size_t j = 0; j < signal.size(); ++j)
            data[i] += signal[j] * codes[i][j];
        data[i] /= static_cast<int>(signal.size());
    }
    return data;
}
