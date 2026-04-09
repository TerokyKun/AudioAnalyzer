#include "BeatDetector.h"

#include <algorithm>

void BeatDetector::Reset()
{
    history = 0.0f;
}

bool BeatDetector::DetectEnergy(float energy)
{
    const float baseline = history * 0.92f + energy * 0.08f;
    const bool beat = (energy > baseline * 1.35f) && (energy > 0.02f);

    history = baseline;
    return beat;
}

bool BeatDetector::Detect(const std::vector<float>& spectrum)
{
    if (spectrum.empty())
        return false;

    float energy = 0.0f;
    const size_t count = std::max<size_t>(1, spectrum.size() / 5);

    for (size_t i = 0; i < count; ++i)
        energy += spectrum[i];

    energy /= static_cast<float>(count);
    return DetectEnergy(energy);
}