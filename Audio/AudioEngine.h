#pragma once

#include <atomic>
#include <string>
#include <vector>
#include "miniaudio.h"

class AudioEngine
{
public:
    AudioEngine() = default;
    ~AudioEngine();

    bool Load(const std::string& path);
    void Play();
    void Pause();
    void Resume();
    void Stop();
    void SeekToSec(double seconds);

    const std::vector<float>& GetSamples() const { return samples; }
    unsigned int GetSampleRate() const { return sampleRate; }
    unsigned int GetChannels() const { return channels; }
    double GetDurationSec() const { return durationSec; }
    float GetPlaybackTimeSec() const;

private:
    ma_decoder decoder{};
    ma_device device{};
    bool decoderReady = false;
    bool deviceReady = false;
    bool playing = false;

    unsigned int sampleRate = 0;
    unsigned int channels = 0;
    double durationSec = 0.0;

    std::vector<float> samples;
    std::atomic<ma_uint64> playedFrames{0};

    static void DataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};