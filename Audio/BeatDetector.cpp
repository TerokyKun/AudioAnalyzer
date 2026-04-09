#include "BeatDetector.h"

bool BeatDetecror::Detect(const std::vector<float> &spectrum)
{
    float energy = 0;

    for (int i = 0; i < spectrum.size() * 0.2; i++)
        energy += spectrum[i];

    energy /= spectrum.size();

    bool bet = energy > energyHistory * 1.5f;

    energyHistory = energy * 0.9f + energyHistory * 0.1f;
   
    return beat;
}