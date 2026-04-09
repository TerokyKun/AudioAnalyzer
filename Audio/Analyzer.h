#pragma once

#include <atomic>
#include <string>
#include <vector>

struct FrameData
{
    float t = 0.0f;
    float bass = 0.0f;
    float mid = 0.0f;
    float high = 0.0f;
    float energy = 0.0f;
    bool beat = false;
};

class Analyzer
{
public:
    void Process(const std::vector<float>& samples, int sampleRate, int channels);
    void SaveJSON(const std::string& path) const;

    float GetProgress() const;
    const std::vector<FrameData>& GetFrames() const { return frames; }

private:
    int sampleRateValue = 0;
    int channelsValue = 2;
    int windowSize = 1024;
    int hopSize = 512;
    double durationSec = 0.0;

    std::atomic<size_t> processedWindows{0};
    size_t totalWindows = 0;

    std::vector<FrameData> frames;
};