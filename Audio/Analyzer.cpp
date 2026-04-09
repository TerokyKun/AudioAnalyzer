#include "Analyzer.h"

#include "BeatDetector.h"
#include "FFT.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

namespace
{
    struct RawFrame
    {
        float t = 0.0f;
        float bass = 0.0f;
        float mid = 0.0f;
        float high = 0.0f;
        float energy = 0.0f;
    };

    static float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    static std::vector<float> MakeHannWindow(size_t n)
    {
        std::vector<float> w(n, 1.0f);
        if (n < 2)
            return w;

        constexpr float PI = 3.14159265358979323846f;
        for (size_t i = 0; i < n; ++i)
        {
            const float x = static_cast<float>(i) / static_cast<float>(n - 1);
            w[i] = 0.5f * (1.0f - std::cos(2.0f * PI * x));
        }
        return w;
    }

    static size_t AdaptiveThreadCount(size_t jobs)
    {
        unsigned int hw = std::thread::hardware_concurrency();
        if (hw == 0)
            hw = 8;

        size_t desired = std::max<size_t>(8, hw);
        if (jobs < desired)
            desired = jobs;

        return std::max<size_t>(1, desired);
    }

    static void AppendFloat(std::string& out, float v, int precision = 6)
    {
        char buf[64];
        const int n = std::snprintf(buf, sizeof(buf), "%.*f", precision, v);
        if (n > 0)
            out.append(buf, static_cast<size_t>(n));
        else
            out.append("0.000000");
    }

    static void AppendUInt(std::string& out, unsigned int v)
    {
        char buf[32];
        const int n = std::snprintf(buf, sizeof(buf), "%u", v);
        if (n > 0)
            out.append(buf, static_cast<size_t>(n));
    }
}

void Analyzer::Process(const std::vector<float>& samples, int sampleRate, int channels)
{
    sampleRateValue = std::max(1, sampleRate);
    channelsValue = std::max(1, channels);
    processedWindows.store(0, std::memory_order_relaxed);
    frames.clear();

    const size_t totalFrames = samples.size() / static_cast<size_t>(channelsValue);
    if (totalFrames == 0)
    {
        totalWindows = 0;
        durationSec = 0.0;
        return;
    }

    const size_t window = static_cast<size_t>(windowSize);
    const size_t hop = static_cast<size_t>(hopSize);

    totalWindows = (totalFrames <= window) ? 1 : (1 + (totalFrames - window) / hop);
    durationSec = static_cast<double>(totalFrames) / static_cast<double>(sampleRateValue);

    std::vector<RawFrame> raw(totalWindows);
    const std::vector<float> hann = MakeHannWindow(window);

    const size_t threadCount = AdaptiveThreadCount(totalWindows);
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (size_t t = 0; t < threadCount; ++t)
    {
        const size_t begin = (totalWindows * t) / threadCount;
        const size_t end = (totalWindows * (t + 1)) / threadCount;

        workers.emplace_back([&, begin, end]()
        {
            std::vector<float> mono(window, 0.0f);
            std::vector<float> spectrum;

            for (size_t idx = begin; idx < end; ++idx)
            {
                const size_t frameStart = idx * hop;

                for (size_t n = 0; n < window; ++n)
                {
                    const size_t srcFrame = frameStart + n;
                    float mix = 0.0f;

                    if (srcFrame < totalFrames)
                    {
                        const size_t base = srcFrame * static_cast<size_t>(channelsValue);
                        for (int ch = 0; ch < channelsValue; ++ch)
                            mix += samples[base + static_cast<size_t>(ch)];

                        mix /= static_cast<float>(channelsValue);
                    }

                    mono[n] = mix * hann[n];
                }

                FFT::Compute(mono, spectrum);

                float bass = 0.0f;
                float mid = 0.0f;
                float high = 0.0f;
                float energy = 0.0f;

                const size_t bins = spectrum.size();
                const size_t bassEnd = static_cast<size_t>(bins * 0.10f);
                const size_t midEnd = static_cast<size_t>(bins * 0.40f);

                for (size_t j = 0; j < bins; ++j)
                {
                    const float v = spectrum[j];
                    energy += v;

                    if (j < bassEnd)
                        bass += v;
                    else if (j < midEnd)
                        mid += v;
                    else
                        high += v;
                }

                const float invBins = bins > 0 ? 1.0f / static_cast<float>(bins) : 1.0f;
                raw[idx] = {
                    static_cast<float>(frameStart) / static_cast<float>(sampleRateValue),
                    bass * invBins,
                    mid * invBins,
                    high * invBins,
                    energy * invBins
                };

                processedWindows.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& worker : workers)
        worker.join();

    float maxBass = 0.0f;
    float maxMid = 0.0f;
    float maxHigh = 0.0f;
    float maxEnergy = 0.0f;

    for (const auto& r : raw)
    {
        maxBass = std::max(maxBass, r.bass);
        maxMid = std::max(maxMid, r.mid);
        maxHigh = std::max(maxHigh, r.high);
        maxEnergy = std::max(maxEnergy, r.energy);
    }

    if (maxBass <= 1e-9f) maxBass = 1.0f;
    if (maxMid <= 1e-9f) maxMid = 1.0f;
    if (maxHigh <= 1e-9f) maxHigh = 1.0f;
    if (maxEnergy <= 1e-9f) maxEnergy = 1.0f;

    frames.resize(totalWindows);

    BeatDetector beatDetector;
    beatDetector.Reset();

    for (size_t i = 0; i < totalWindows; ++i)
    {
        const auto& r = raw[i];

        const float bass = Clamp01(r.bass / maxBass);
        const float mid = Clamp01(r.mid / maxMid);
        const float high = Clamp01(r.high / maxHigh);
        const float energy = Clamp01(r.energy / maxEnergy);

        const bool beat = beatDetector.DetectEnergy(std::max(0.0f, energy * 0.75f + bass * 0.25f));

        frames[i] = {
            r.t,
            bass,
            mid,
            high,
            energy,
            beat
        };
    }
}

float Analyzer::GetProgress() const
{
    if (totalWindows == 0)
        return 0.0f;

    const size_t done = processedWindows.load(std::memory_order_relaxed);
    return Clamp01(static_cast<float>(done) / static_cast<float>(totalWindows));
}

void Analyzer::SaveJSON(const std::string& path) const
{
    std::filesystem::path outPath(path);
    if (outPath.has_parent_path())
        std::filesystem::create_directories(outPath.parent_path());

    std::string json;
    json.reserve(frames.size() * 128 + 512);

    json += "{\n";
    json += "  \"metadata\": {\n";
    json += "    \"sample_rate\": ";
    AppendUInt(json, static_cast<unsigned int>(sampleRateValue));
    json += ",\n    \"channels\": ";
    AppendUInt(json, static_cast<unsigned int>(channelsValue));
    json += ",\n    \"window_size\": ";
    AppendUInt(json, static_cast<unsigned int>(windowSize));
    json += ",\n    \"hop_size\": ";
    AppendUInt(json, static_cast<unsigned int>(hopSize));
    json += ",\n    \"duration_sec\": ";
    AppendFloat(json, static_cast<float>(durationSec), 6);
    json += ",\n    \"frame_count\": ";
    AppendUInt(json, static_cast<unsigned int>(frames.size()));
    json += "\n  },\n";

    json += "  \"frames\": [\n";
    for (size_t i = 0; i < frames.size(); ++i)
    {
        const FrameData& f = frames[i];

        json += "    {\"t\":";
        AppendFloat(json, f.t);
        json += ",\"bass\":";
        AppendFloat(json, f.bass);
        json += ",\"mid\":";
        AppendFloat(json, f.mid);
        json += ",\"high\":";
        AppendFloat(json, f.high);
        json += ",\"energy\":";
        AppendFloat(json, f.energy);
        json += ",\"beat\":";
        json += (f.beat ? "true" : "false");
        json += "}";

        if (i + 1 < frames.size())
            json += ",";

        json += "\n";
    }
    json += "  ]\n";
    json += "}\n";

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to save JSON: " << path << "\n";
        return;
    }

    file.write(json.data(), static_cast<std::streamsize>(json.size()));
    std::cout << "JSON saved to " << path << "\n";
}