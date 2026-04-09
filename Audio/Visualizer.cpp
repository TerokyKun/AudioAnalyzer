#include "Visualizer.h"

#include <algorithm>
#include <cmath>
#include <glad/glad.h>

namespace
{
    static float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    static float Hash01(float x)
    {
        const float v = std::sin(x * 12.9898f + 78.233f) * 43758.5453f;
        return v - std::floor(v);
    }

    static void DrawRect(float cx, float cy, float w, float h, float angleDeg, float r, float g, float b, float a)
    {
        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);
        glRotatef(angleDeg, 0.0f, 0.0f, 1.0f);
        glColor4f(r, g, b, a);

        glBegin(GL_QUADS);
        glVertex2f(-w, -h);
        glVertex2f(w, -h);
        glVertex2f(w, h);
        glVertex2f(-w, h);
        glEnd();

        glPopMatrix();
    }

    static void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glEnd();
    }

    static void DrawQuad(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x0, y0);
        glVertex2f(x1, y0);
        glVertex2f(x1, y1);
        glVertex2f(x0, y1);
        glEnd();
    }
}

void Visualizer::Draw(float bass, float mid, float high, bool beat)
{
    const float b = Clamp01(bass);
    const float m = Clamp01(mid);
    const float h = Clamp01(high);

    const float size = 0.18f + b * 0.55f + (beat ? 0.08f : 0.0f);
    DrawRect(0.0f, 0.0f, size, size, beat ? 12.0f : 0.0f, m, 0.15f + b * 0.25f, h, 0.9f);
}

void Visualizer::DrawProgress(float progress)
{
    const float p = Clamp01(progress);

    DrawQuad(-1.0f, -0.95f, 1.0f, -0.90f, 0.08f, 0.08f, 0.10f, 1.0f);
    DrawQuad(-1.0f, -0.95f, -1.0f + 2.0f * p, -0.90f, 0.20f + p * 0.6f, 0.75f, 0.25f, 1.0f);
}

void Visualizer::DrawLoading(float progress)
{
    const float p = Clamp01(progress);

    DrawQuad(-1.0f, -0.95f, 1.0f, -0.90f, 0.08f, 0.08f, 0.10f, 1.0f);
    DrawQuad(-1.0f, -0.95f, -1.0f + 2.0f * p, -0.90f, 0.20f, 0.80f, 0.30f, 1.0f);

    const float pulse = 0.18f + p * 0.42f;
    DrawRect(0.0f, 0.0f, pulse, pulse, p * 120.0f, 0.18f + p * 0.5f, 0.25f + p * 0.4f, 0.65f + p * 0.3f, 0.9f);
}

void Visualizer::DrawFrame(const FrameData& frame, float playbackSec, float songProgress, float beatPulse, bool dragging, float scrubProgress, std::size_t frameIndex)
{
    const float bass = Clamp01(frame.bass);
    const float mid = Clamp01(frame.mid);
    const float high = Clamp01(frame.high);
    const float energy = Clamp01(frame.energy);

    const float t = playbackSec;

    const float baseR = 0.04f + energy * 0.16f + 0.03f * std::sin(t * 0.7f);
    const float baseG = 0.04f + bass * 0.08f + 0.02f * std::sin(t * 1.1f);
    const float baseB = 0.07f + high * 0.18f + 0.03f * std::sin(t * 0.9f);
    glClearColor(std::clamp(baseR, 0.0f, 1.0f), std::clamp(baseG, 0.0f, 1.0f), std::clamp(baseB, 0.0f, 1.0f), 1.0f);

    const float flash = Clamp01(beatPulse);
    if (flash > 0.001f)
    {
        DrawQuad(-1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 0.7f + bass * 0.2f, 0.25f, flash * 0.20f);
    }

    const float bassScale = 0.18f + bass * 0.58f + flash * 0.08f;
    const float bassRot = t * (18.0f + bass * 70.0f);

    glPushMatrix();
    glRotatef(bassRot, 0.0f, 0.0f, 1.0f);
    DrawRect(0.0f, 0.0f, bassScale, bassScale, 0.0f, 0.24f + bass * 0.70f, 0.12f + mid * 0.25f, 0.32f + high * 0.55f, 0.92f);
    glPopMatrix();

    const int orbitCount = 4 + static_cast<int>(mid * 10.0f);
    const float orbitRadius = 0.28f + mid * 0.32f;
    const float orbitSpin = t * (0.9f + mid * 1.8f);

    for (int i = 0; i < orbitCount; ++i)
    {
        const float seed = static_cast<float>(frameIndex) * 19.0f + static_cast<float>(i) * 7.3f;
        const float jitter = Hash01(seed) * 0.8f;
        const float ang = orbitSpin + (6.2831853f * static_cast<float>(i) / static_cast<float>(orbitCount)) + jitter;
        const float x = std::cos(ang) * orbitRadius;
        const float y = std::sin(ang) * orbitRadius;

        const float w = 0.02f + mid * 0.03f;
        const float h = 0.08f + mid * 0.16f;

        DrawRect(x, y, w, h, ang * 57.29578f + 90.0f, 0.20f + mid * 0.60f, 0.35f + Hash01(seed + 1.7f) * 0.4f, 0.35f + high * 0.45f, 0.78f);
    }

    const int spikes = 10 + static_cast<int>(high * 20.0f);
    const float spikeLen = 0.18f + high * 0.44f + energy * 0.10f;
    const float spikeSpin = t * (0.45f + high * 1.4f);

    for (int i = 0; i < spikes; ++i)
    {
        const float seed = static_cast<float>(frameIndex) * 31.0f + static_cast<float>(i) * 11.0f;
        const float a = spikeSpin + (6.2831853f * static_cast<float>(i) / static_cast<float>(spikes));
        const float len = spikeLen * (0.6f + Hash01(seed) * 0.8f);
        const float x2 = std::cos(a) * len;
        const float y2 = std::sin(a) * len;

        DrawLine(0.0f, 0.0f, x2, y2, 0.4f + high * 0.6f, 0.6f + Hash01(seed + 1.3f) * 0.3f, 1.0f, 0.35f + high * 0.45f);
    }

    const int gridLines = 6 + static_cast<int>(energy * 10.0f);
    for (int i = 0; i < gridLines; ++i)
    {
        const float seed = static_cast<float>(frameIndex) * 13.0f + static_cast<float>(i) * 3.7f;
        const float x = -0.95f + Hash01(seed) * 1.90f;
        const float y = -0.75f + Hash01(seed + 2.1f) * 1.50f;
        const float len = 0.15f + Hash01(seed + 4.2f) * 0.55f;
        DrawLine(x, y, x + len, y, 0.15f + energy * 0.35f, 0.15f + high * 0.25f, 0.20f + mid * 0.25f, 0.18f);
    }

    const float pulseSize = 0.05f + flash * 0.12f + energy * 0.05f;
    DrawRect(0.0f, 0.0f, pulseSize, pulseSize, t * 180.0f, 1.0f, 0.9f, 0.55f, 0.95f);

    const float barProgress = dragging ? Clamp01(scrubProgress) : Clamp01(songProgress);
    DrawQuad(-1.0f, -0.95f, 1.0f, -0.90f, 0.06f, 0.06f, 0.08f, 1.0f);
    DrawQuad(-1.0f, -0.95f, -1.0f + 2.0f * barProgress, -0.90f, 0.20f + bass * 0.5f, 0.75f, 0.25f + high * 0.4f, 1.0f);

    const float handleX = -1.0f + 2.0f * barProgress;
    DrawQuad(handleX - 0.008f, -0.97f, handleX + 0.008f, -0.88f, 1.0f, 0.6f, 0.2f, 1.0f);
}