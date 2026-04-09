#pragma once
#include <vector>

class BeatDetector
{
    public:
    bool Detect(const std::vector<float>& spectrum);
    private:
    float energyHistory = 0;
};