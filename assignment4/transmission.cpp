#include "cdma.h"
#include <stdexcept>
using namespace std;

vector<int> transmit(const vector<int>& data, const vector<vector<int>>& codes) {
    if (codes.empty() || codes[0].empty() || data.size() != codes.size())
        throw invalid_argument("Provide one nonempty code per station.");
    vector<int> signal(codes[0].size(), 0);
    for (size_t i = 0; i < data.size(); ++i) {
        if (codes[i].size() != signal.size() || data[i] < -1 || data[i] > 1)
            throw invalid_argument("Use equal code lengths and data values -1, 0, +1.");
        for (size_t j = 0; j < signal.size(); ++j)
            signal[j] += data[i] * codes[i][j];
    }
    return signal;
}
