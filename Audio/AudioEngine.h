#pragma once
#include <vector>
#include <string>

class AudioEngine
{
public:
    bool Load(const std::string &path);

    const std::vector<float> &GetSamples() const;
    int GetSampleRate() const;
    int GetChannels() const;

private:
    std::vector<float> samples;
    int sampleRate = 44100;
    int channels = 1;
};