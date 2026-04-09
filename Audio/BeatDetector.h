#pragma once

#include <vector>

class BeatDetector
{
public:
    void Reset();
    bool DetectEnergy(float energy);
    bool Detect(const std::vector<float>& spectrum);

private:
    float history = 0.0f;
};