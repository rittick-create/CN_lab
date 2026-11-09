#pragma once
#include <vector>

std::vector<std::vector<int>> walsh(int n);

//data contains +1 (bit 1), -1 (bit 0), or 0 (silent).
std::vector<int> transmit(const std::vector<int>& data,const std::vector<std::vector<int>>& codes);

// Returns each station's data value: +1, -1, or 0.
std::vector<int> receive(const std::vector<int>& signal,const std::vector<std::vector<int>>& codes);
