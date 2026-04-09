#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "AudioEngine.h"

#include <iostream>

bool AudioEngine::Load(const std::string& path)
{
    ma_decoder decoder;

    if (ma_decoder_init_file(path.c_str(), NULL, &decoder) != MA_SUCCESS)
    {
        std::cout << "Error load\n";
        return false;
    }

    sampleRate = decoder.outputSampleRate;
    channels = decoder.outputChannels;

    ma_uint64 frameCount;
    ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);

    samples.resize(frameCount * channels);

    ma_decoder_read_pcm_frames(&decoder, samples.data(), frameCount, NULL);

    ma_decoder_uninit(&decoder);

    std::cout << "Loaded: " << path << "\n";
    std::cout << "SampleRate: " << sampleRate << " Channels: " << channels << "\n";

    return true;
}

const std::vector<float>& AudioEngine::GetSamples() const { return samples; }
int AudioEngine::GetSampleRate() const { return sampleRate; }
int AudioEngine::GetChannels() const { return channels; }