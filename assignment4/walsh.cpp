#include "cdma.h"
#include <limits>
#include <stdexcept>
using namespace std;

vector<vector<int>> walsh(int n) {
    if (n < 1) throw invalid_argument("Station count must be positive.");
    int size = 1;
    while (size < n) {
        if (size > numeric_limits<int>::max() / 2)
            throw invalid_argument("Too many stations.");
        size *= 2;
    }
    vector<vector<int>> w(size, vector<int>(size, 1));
    for (int k = 1; k < size; k *= 2)
        for (int i = 0; i < k; ++i)
            for (int j = 0; j < k; ++j) {
                w[i + k][j] = w[i][j + k] = w[i][j];
                w[i + k][j + k] = -w[i][j];
            }
    w.resize(n);
    return w;
}
