#pragma once

#include <vector>

class FFT
{
public:
    static void Compute(const std::vector<float>& input, std::vector<float>& output);
};