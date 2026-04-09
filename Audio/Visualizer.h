#pragma once

#include <cstddef>
#include <vector>

struct FrameData;

class Visualizer
{
public:
    void Draw(float bass, float mid, float high, bool beat);
    void DrawProgress(float progress);
    void DrawLoading(float progress);

    void DrawFrame(const FrameData& frame,
                   float playbackSec,
                   float songProgress,
                   float beatPulse,
                   bool dragging,
                   float scrubProgress,
                   std::size_t frameIndex);

public:
    struct Object3D
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        float vx = 0.0f;
        float vy = 0.0f;
        float vz = 0.0f;

        float rx = 0.0f;
        float ry = 0.0f;
        float rz = 0.0f;

        float vrx = 0.0f;
        float vry = 0.0f;
        float vrz = 0.0f;

        float size = 0.1f;
        float seed = 0.0f;
        int type = 0; // 0 sphere, 1 cone, 2 cube
    };

    std::vector<Object3D> objects;
    bool initialized = false;

    float smBass = 0.0f;
    float smMid = 0.0f;
    float smHigh = 0.0f;
    float smEnergy = 0.0f;

    float prevBass = 0.0f;
    float prevMid = 0.0f;
    float prevHigh = 0.0f;
    float prevEnergy = 0.0f;

    float shakeX = 0.0f;
    float shakeY = 0.0f;
    float glitch = 0.0f;
    float lavaPhase = 0.0f;
};