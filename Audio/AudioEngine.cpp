#include "AudioEngine.h"

#include <cstring>
#include <iostream>
#include <algorithm>

AudioEngine::~AudioEngine()
{
    Stop();

    if (deviceReady)
    {
        ma_device_uninit(&device);
        deviceReady = false;
    }

    if (decoderReady)
    {
        ma_decoder_uninit(&decoder);
        decoderReady = false;
    }
}

bool AudioEngine::Load(const std::string& path)
{
    Stop();

    if (deviceReady)
    {
        ma_device_uninit(&device);
        deviceReady = false;
    }

    if (decoderReady)
    {
        ma_decoder_uninit(&decoder);
        decoderReady = false;
    }

    if (ma_decoder_init_file(path.c_str(), nullptr, &decoder) != MA_SUCCESS)
    {
        std::cerr << "Failed to load audio: " << path << "\n";
        return false;
    }

    decoderReady = true;

    sampleRate = decoder.outputSampleRate;
    channels = decoder.outputChannels;

    ma_uint64 frameCount = 0;
    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS)
    {
        std::cerr << "Failed to get frame count.\n";
        ma_decoder_uninit(&decoder);
        decoderReady = false;
        return false;
    }

    if (frameCount == 0 || channels == 0)
    {
        std::cerr << "Audio file is empty or invalid.\n";
        ma_decoder_uninit(&decoder);
        decoderReady = false;
        return false;
    }

    samples.resize(static_cast<size_t>(frameCount) * static_cast<size_t>(channels));

    ma_uint64 framesRead = ma_decoder_read_pcm_frames(&decoder, samples.data(), frameCount, nullptr);
    if (framesRead != frameCount)
    {
        std::cerr << "Warning: not all frames were read.\n";
    }

    durationSec = sampleRate > 0 ? static_cast<double>(frameCount) / static_cast<double>(sampleRate) : 0.0;

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = decoder.outputFormat;
    config.playback.channels = decoder.outputChannels;
    config.sampleRate = decoder.outputSampleRate;
    config.dataCallback = AudioEngine::DataCallback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &device) != MA_SUCCESS)
    {
        std::cerr << "Failed to init playback device.\n";
        ma_decoder_uninit(&decoder);
        decoderReady = false;
        return false;
    }

    deviceReady = true;
    playing = false;
    playedFrames.store(0, std::memory_order_relaxed);

    std::cout << "Loaded: " << path << "\n";
    std::cout << "SampleRate: " << sampleRate << " Channels: " << channels << "\n";

    return true;
}

void AudioEngine::Play()
{
    if (!decoderReady || !deviceReady)
        return;

    SeekToSec(0.0);

    if (ma_device_start(&device) != MA_SUCCESS)
    {
        std::cerr << "Failed to start playback device.\n";
        playing = false;
        return;
    }

    playing = true;
}

void AudioEngine::Pause()
{
    if (!deviceReady || !playing)
        return;

    ma_device_stop(&device);
    playing = false;
}

void AudioEngine::Resume()
{
    if (!deviceReady || playing)
        return;

    if (ma_device_start(&device) != MA_SUCCESS)
    {
        std::cerr << "Failed to resume playback device.\n";
        return;
    }

    playing = true;
}

void AudioEngine::Stop()
{
    if (deviceReady)
        ma_device_stop(&device);

    playing = false;
}

void AudioEngine::SeekToSec(double seconds)
{
    if (!decoderReady)
        return;

    const bool wasPlaying = playing;
    if (wasPlaying)
        Pause();

    const double clamped = std::clamp(seconds, 0.0, durationSec);
    const ma_uint64 targetFrame = static_cast<ma_uint64>(clamped * static_cast<double>(sampleRate));

    ma_decoder_seek_to_pcm_frame(&decoder, targetFrame);
    playedFrames.store(targetFrame, std::memory_order_relaxed);

    if (wasPlaying)
        Resume();
}

float AudioEngine::GetPlaybackTimeSec() const
{
    if (sampleRate == 0)
        return 0.0f;

    const ma_uint64 frames = playedFrames.load(std::memory_order_relaxed);
    return static_cast<float>(static_cast<double>(frames) / static_cast<double>(sampleRate));
}

void AudioEngine::DataCallback(ma_device* pDevice, void* pOutput, const void* /*pInput*/, ma_uint32 frameCount)
{
    AudioEngine* engine = static_cast<AudioEngine*>(pDevice->pUserData);
    if (!engine)
        return;

    const ma_uint32 bytesPerFrame = ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
    std::memset(pOutput, 0, static_cast<size_t>(bytesPerFrame) * frameCount);

    ma_uint64 framesRead = ma_decoder_read_pcm_frames(&engine->decoder, pOutput, frameCount, nullptr);
    engine->playedFrames.fetch_add(framesRead, std::memory_order_relaxed);
}