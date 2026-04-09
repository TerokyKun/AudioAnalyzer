#include "Visualizer.h"
#include "Analyzer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <glad/glad.h>

namespace
{
    constexpr float PI = 3.14159265358979323846f;

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

    static float SmoothAttackRelease(float current, float previous, float attack, float release)
    {
        return (current > previous)
            ? previous + (current - previous) * attack
            : previous + (current - previous) * release;
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

    static void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_LINES);
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
        glEnd();
    }

    static void DrawBlob(float cx, float cy, float radius, float r, float g, float b, float a, int segments = 40)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(cx, cy);

        for (int i = 0; i <= segments; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(segments);
            float ang = t * 2.0f * PI;
            float wobble = 1.0f + 0.08f * std::sin(ang * 3.0f + cx * 7.0f + cy * 5.0f);
            glVertex2f(cx + std::cos(ang) * radius * wobble, cy + std::sin(ang) * radius * wobble);
        }

        glEnd();
    }

    static void ProjectIso(float x, float y, float z, float rotY, float& outX, float& outY)
    {
        float c = std::cos(rotY);
        float s = std::sin(rotY);

        float xr = x * c + z * s;
        float zr = -x * s + z * c;

        outX = xr + zr * 0.5f;
        outY = y - zr * 0.25f;
    }

    static void DrawCubeIso(float cx, float cy, float size, float rotY, float rotX, float rotZ, float r, float g, float b, float alpha)
    {
        float hx = size * 0.5f;
        float hy = size * 0.5f;
        float hz = size * 0.5f;

        float vx[8];
        float vy[8];

        const float pts[8][3] = {
            {-hx, -hy, -hz},
            { hx, -hy, -hz},
            { hx,  hy, -hz},
            {-hx,  hy, -hz},
            {-hx, -hy,  hz},
            { hx, -hy,  hz},
            { hx,  hy,  hz},
            {-hx,  hy,  hz},
        };

        for (int i = 0; i < 8; ++i)
        {
            float x = pts[i][0];
            float y = pts[i][1];
            float z = pts[i][2];

            float cz = std::cos(rotZ);
            float sz = std::sin(rotZ);
            float xr = x * cz - y * sz;
            float yr = x * sz + y * cz;
            float zr = z;

            float cy = std::cos(rotY);
            float sy = std::sin(rotY);
            float x2 = xr * cy + zr * sy;
            float z2 = -xr * sy + zr * cy;

            float cxr = std::cos(rotX);
            float sxr = std::sin(rotX);
            float y2 = yr * cxr - z2 * sxr;
            float z3 = yr * sxr + z2 * cxr;

            vx[i] = cx + x2 + z3 * 0.45f;
            vy[i] = cy + y2 - z3 * 0.20f;
        }

        glBegin(GL_QUADS);
        glColor4f(r * 0.55f, g * 0.55f, b * 0.55f, alpha * 0.95f);
        glVertex2f(vx[0], vy[0]);
        glVertex2f(vx[1], vy[1]);
        glVertex2f(vx[2], vy[2]);
        glVertex2f(vx[3], vy[3]);

        glColor4f(r * 0.80f, g * 0.80f, b * 0.80f, alpha * 0.90f);
        glVertex2f(vx[4], vy[4]);
        glVertex2f(vx[5], vy[5]);
        glVertex2f(vx[6], vy[6]);
        glVertex2f(vx[7], vy[7]);

        glColor4f(r, g, b, alpha);
        glVertex2f(vx[3], vy[3]);
        glVertex2f(vx[2], vy[2]);
        glVertex2f(vx[6], vy[6]);
        glVertex2f(vx[7], vy[7]);
        glEnd();

        glBegin(GL_LINES);
        glColor4f(1.0f, 1.0f, 1.0f, alpha * 0.55f);
        const int edges[12][2] = {
            {0,1},{1,2},{2,3},{3,0},
            {4,5},{5,6},{6,7},{7,4},
            {0,4},{1,5},{2,6},{3,7}
        };
        for (auto& e : edges)
        {
            glVertex2f(vx[e[0]], vy[e[0]]);
            glVertex2f(vx[e[1]], vy[e[1]]);
        }
        glEnd();
    }

    static void DrawConeIso(float cx, float cy, float size, float rotY, float rotX, float rotZ, float r, float g, float b, float alpha)
    {
        const float h = size * 1.25f;
        const float rad = size * 0.60f;

        auto proj = [&](float x, float y, float z, float& outX, float& outY)
        {
            float cz = std::cos(rotZ);
            float sz = std::sin(rotZ);
            float xr = x * cz - y * sz;
            float yr = x * sz + y * cz;
            float zr = z;

            float cy = std::cos(rotY);
            float sy = std::sin(rotY);
            float x2 = xr * cy + zr * sy;
            float z2 = -xr * sy + zr * cy;

            float cxr = std::cos(rotX);
            float sxr = std::sin(rotX);
            float y2 = yr * cxr - z2 * sxr;
            float z3 = yr * sxr + z2 * cxr;

            outX = x2 + z3 * 0.45f;
            outY = y2 - z3 * 0.20f;
        };

        float topX, topY;
        float b1x, b1y, b2x, b2y, b3x, b3y, b4x, b4y;

        proj(0.0f, h * 0.55f, 0.0f, topX, topY);
        proj(-rad, -h * 0.45f, -rad, b1x, b1y);
        proj( rad, -h * 0.45f, -rad, b2x, b2y);
        proj( rad, -h * 0.45f,  rad, b3x, b3y);
        proj(-rad, -h * 0.45f,  rad, b4x, b4y);

        topX += cx; topY += cy;
        b1x += cx; b1y += cy;
        b2x += cx; b2y += cy;
        b3x += cx; b3y += cy;
        b4x += cx; b4y += cy;

        glBegin(GL_TRIANGLES);
        glColor4f(r, g, b, alpha);
        glVertex2f(topX, topY);
        glVertex2f(b1x, b1y);
        glVertex2f(b2x, b2y);

        glColor4f(r * 0.90f, g * 0.90f, b * 0.90f, alpha * 0.95f);
        glVertex2f(topX, topY);
        glVertex2f(b2x, b2y);
        glVertex2f(b3x, b3y);

        glColor4f(r * 0.80f, g * 0.80f, b * 0.80f, alpha * 0.90f);
        glVertex2f(topX, topY);
        glVertex2f(b3x, b3y);
        glVertex2f(b4x, b4y);

        glColor4f(r * 0.72f, g * 0.72f, b * 0.72f, alpha * 0.85f);
        glVertex2f(topX, topY);
        glVertex2f(b4x, b4y);
        glVertex2f(b1x, b1y);
        glEnd();

        glBegin(GL_LINE_LOOP);
        glColor4f(1.0f, 1.0f, 1.0f, alpha * 0.55f);
        glVertex2f(b1x, b1y);
        glVertex2f(b2x, b2y);
        glVertex2f(b3x, b3y);
        glVertex2f(b4x, b4y);
        glEnd();
    }

    static void DrawSphereLite(float cx, float cy, float radius, float rotX, float rotY, float rotZ, float r, float g, float b, float a)
    {
        const int rings = 6;
        const int segs = 14;

        for (int ring = 0; ring < rings; ++ring)
        {
            float t0 = static_cast<float>(ring) / static_cast<float>(rings);
            float t1 = static_cast<float>(ring + 1) / static_cast<float>(rings);

            float rr0 = std::sin(t0 * PI);
            float rr1 = std::sin(t1 * PI);

            float yy0 = std::cos(t0 * PI);
            float yy1 = std::cos(t1 * PI);

            glBegin(GL_TRIANGLE_STRIP);
            for (int s = 0; s <= segs; ++s)
            {
                float u = static_cast<float>(s) / static_cast<float>(segs);
                float ang = u * 2.0f * PI + rotZ;

                float x0 = std::cos(ang) * rr0;
                float z0 = std::sin(ang) * rr0;

                float x1 = std::cos(ang) * rr1;
                float z1 = std::sin(ang) * rr1;

                float cy = std::cos(rotY);
                float sy = std::sin(rotY);

                float rx0 = x0 * cy + z0 * sy;
                float rz0 = -x0 * sy + z0 * cy;

                float rx1 = x1 * cy + z1 * sy;
                float rz1 = -x1 * sy + z1 * cy;

                float cxr = std::cos(rotX);
                float sxr = std::sin(rotX);

                float sx0 = rx0;
                float sy0 = yy0 * cxr - rz0 * sxr;
                float depth0 = 1.0f / (1.0f + (rz0 + 1.0f) * 0.35f);

                float sx1 = rx1;
                float sy1 = yy1 * cxr - rz1 * sxr;
                float depth1 = 1.0f / (1.0f + (rz1 + 1.0f) * 0.35f);

                glColor4f(r * depth0, g * depth0, b * depth0, a);
                glVertex2f(cx + sx0 * radius, cy + sy0 * radius);

                glColor4f(r * depth1, g * depth1, b * depth1, a);
                glVertex2f(cx + sx1 * radius, cy + sy1 * radius);
            }
            glEnd();
        }
    }

    static void DrawHexRing(float cx, float cy, float radius, float rot, float r, float g, float b, float a)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 6; ++i)
        {
            float ang = rot + (2.0f * PI * static_cast<float>(i) / 6.0f);
            glVertex2f(cx + std::cos(ang) * radius, cy + std::sin(ang) * radius);
        }
        glEnd();
    }

    static void DrawScanlines(float t, float high, float glitch)
    {
        for (int i = 0; i < 54; ++i)
        {
            float y = -1.0f + (static_cast<float>(i) / 53.0f) * 2.0f;
            float wobble = std::sin(y * 32.0f + t * 8.0f + high * 12.0f) * (0.008f + high * 0.028f);
            float alpha = 0.018f + high * 0.040f + glitch * 0.012f;

            DrawLine(-1.0f + wobble, y, 1.0f + wobble, y, 0.92f, 0.96f, 1.0f, alpha);
        }
    }

    static void DrawNoise(float frameSeed, float high, float energy, float glitch)
    {
        int count = 45 + static_cast<int>(high * 120.0f) + static_cast<int>(glitch * 60.0f);
        for (int i = 0; i < count; ++i)
        {
            float seed = frameSeed * 17.13f + static_cast<float>(i) * 7.91f;
            float x = Hash01(seed) * 2.0f - 1.0f;
            float y = Hash01(seed + 1.3f) * 2.0f - 1.0f;
            float s = 0.0012f + Hash01(seed + 2.6f) * 0.0050f;
            float a = (0.018f + high * 0.050f + energy * 0.018f) * (0.35f + Hash01(seed + 4.9f));

            glColor4f(1.0f, 1.0f, 1.0f, a);
            glBegin(GL_QUADS);
            glVertex2f(x - s, y - s);
            glVertex2f(x + s, y - s);
            glVertex2f(x + s, y + s);
            glVertex2f(x - s, y + s);
            glEnd();
        }
    }

    static void DrawVhsBars(float t, float high, float energy, float glitch)
    {
        int bars = 3 + static_cast<int>(high * 6.0f) + static_cast<int>(glitch * 4.0f);
        for (int i = 0; i < bars; ++i)
        {
            float seed = t * 3.7f + static_cast<float>(i) * 11.3f;
            float y = -0.8f + Hash01(seed) * 1.6f;
            float h = 0.010f + Hash01(seed + 2.0f) * 0.045f;
            float xShift = std::sin(t * 12.0f + seed) * (0.02f + energy * 0.05f);
            float alpha = 0.035f + high * 0.040f + glitch * 0.040f;

            DrawQuad(-1.0f + xShift, y, 1.0f + xShift, y + h, 0.92f, 0.88f, 1.0f, alpha);
        }
    }

    static void DrawChromaticSplit(float cx, float cy, float size, float rot, float bass, float mid, float high, float glitch)
    {
        float offset = 0.02f + glitch * 0.04f;
        float wobble = std::sin(rot * 3.0f) * 0.05f;

        DrawCubeIso(cx - offset, cy, size, rot + wobble, rot * 0.7f, rot * 1.1f, 1.0f, 0.25f + bass * 0.55f, 0.22f, 0.30f);
        DrawCubeIso(cx, cy + offset * 0.3f, size * 0.98f, rot, rot * 0.8f, rot * 0.9f, 0.18f, 1.0f, 0.28f + mid * 0.55f, 0.28f);
        DrawCubeIso(cx + offset, cy - offset * 0.15f, size * 0.96f, rot - wobble, rot * 0.6f, rot * 1.2f, 0.22f, 0.35f + mid * 0.4f, 1.0f, 0.28f + high * 0.10f);
    }

    static void ApplyCollision(Visualizer::Object3D& a, Visualizer::Object3D& b)
    {
        const float dx = b.x - a.x;
        const float dy = b.y - a.y;
        const float dz = b.z - a.z;
        const float distSq = dx * dx + dy * dy + dz * dz;
        const float minDist = a.size + b.size;

        if (distSq <= 0.000001f)
            return;

        const float dist = std::sqrt(distSq);
        if (dist >= minDist)
            return;

        const float nx = dx / dist;
        const float ny = dy / dist;
        const float nz = dz / dist;

        const float push = (minDist - dist) * 0.5f;
        a.x -= nx * push;
        a.y -= ny * push;
        a.z -= nz * push;

        b.x += nx * push;
        b.y += ny * push;
        b.z += nz * push;

        const float impulse = 0.0025f;
        a.vx -= nx * impulse;
        a.vy -= ny * impulse;
        a.vz -= nz * impulse;

        b.vx += nx * impulse;
        b.vy += ny * impulse;
        b.vz += nz * impulse;

        a.vrx += 0.01f * (Hash01(a.seed + 20.0f) - 0.5f);
        a.vry += 0.01f * (Hash01(a.seed + 21.0f) - 0.5f);
        a.vrz += 0.01f * (Hash01(a.seed + 22.0f) - 0.5f);

        b.vrx += 0.01f * (Hash01(b.seed + 20.0f) - 0.5f);
        b.vry += 0.01f * (Hash01(b.seed + 21.0f) - 0.5f);
        b.vrz += 0.01f * (Hash01(b.seed + 22.0f) - 0.5f);
    }
}

void Visualizer::Draw(float bass, float mid, float high, bool beat)
{
    const float b = Clamp01(bass);
    const float m = Clamp01(mid);
    const float h = Clamp01(high);

    const float size = 0.18f + b * 0.55f + (beat ? 0.08f : 0.0f);
    DrawBlob(0.0f, 0.0f, size, 0.25f + b * 0.7f, 0.15f + m * 0.45f, 0.3f + h * 0.65f, 0.92f);
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
    DrawBlob(0.0f, 0.0f, pulse, 0.18f + p * 0.50f, 0.25f + p * 0.35f, 0.65f + p * 0.20f, 0.88f);
}

void Visualizer::DrawFrame(const FrameData& frame,
                           float playbackSec,
                           float songProgress,
                           float beatPulse,
                           bool dragging,
                           float scrubProgress,
                           std::size_t frameIndex)
{
    const float bass = Clamp01(frame.bass);
    const float mid = Clamp01(frame.mid);
    const float high = Clamp01(frame.high);
    const float energy = Clamp01(frame.energy);

    smBass = SmoothAttackRelease(bass, smBass, 0.12f, 0.03f);
    smMid = SmoothAttackRelease(mid, smMid, 0.10f, 0.03f);
    smHigh = SmoothAttackRelease(high, smHigh, 0.10f, 0.03f);
    smEnergy = SmoothAttackRelease(energy, smEnergy, 0.08f, 0.03f);

    const float delta =
        std::fabs(smBass - prevBass) +
        std::fabs(smMid - prevMid) +
        std::fabs(smHigh - prevHigh) +
        std::fabs(smEnergy - prevEnergy);

    prevBass = smBass;
    prevMid = smMid;
    prevHigh = smHigh;
    prevEnergy = smEnergy;

    lavaPhase += 0.004f + smBass * 0.012f + smEnergy * 0.008f;

    float shock = Clamp01(delta * 3.5f + beatPulse * 0.8f);
    glitch = std::max(glitch * 0.94f, shock);

    const float t = playbackSec + lavaPhase * 2.0f;
    const float shakeAmount = (0.0025f + glitch * 0.010f + smEnergy * 0.004f);
    shakeX = std::sin(t * 13.0f + frameIndex * 0.17f) * shakeAmount;
    shakeY = std::cos(t * 11.0f + frameIndex * 0.11f) * shakeAmount * 0.8f;

    if (!initialized)
    {
        for (int i = 0; i < 14; ++i)
        {
            Object3D o;
            o.seed = static_cast<float>(i) * 13.3f;

            o.x = Hash01(o.seed) * 2.0f - 1.0f;
            o.y = Hash01(o.seed + 1.0f) * 1.2f - 0.6f;
            o.z = Hash01(o.seed + 2.0f) * 1.0f;

            o.vx = (Hash01(o.seed + 3.0f) - 0.5f) * 0.0018f;
            o.vy = (Hash01(o.seed + 4.0f) - 0.5f) * 0.0018f;
            o.vz = (Hash01(o.seed + 5.0f) - 0.5f) * 0.0012f;

            o.rx = Hash01(o.seed + 6.0f) * 6.2831853f;
            o.ry = Hash01(o.seed + 7.0f) * 6.2831853f;
            o.rz = Hash01(o.seed + 8.0f) * 6.2831853f;

            o.vrx = (Hash01(o.seed + 9.0f) - 0.5f) * 0.012f;
            o.vry = (Hash01(o.seed + 10.0f) - 0.5f) * 0.012f;
            o.vrz = (Hash01(o.seed + 11.0f) - 0.5f) * 0.012f;

            o.size = 0.08f + Hash01(o.seed + 12.0f) * 0.15f;
            o.type = static_cast<int>(Hash01(o.seed + 13.0f) * 3.0f);

            objects.push_back(o);
        }

        initialized = true;
    }

    for (auto& o : objects)
    {
        const float n1 = std::sin(t * 0.7f + o.seed);
        const float n2 = std::cos(t * 0.6f + o.seed * 0.7f);
        const float n3 = std::sin(t * 0.4f + o.seed * 1.3f);

        o.vx += n1 * 0.0004f + smBass * 0.0015f;
        o.vy += n2 * 0.0004f + smMid * 0.0012f;
        o.vz += n3 * 0.0003f + smHigh * 0.0009f;

        if (shock > 0.18f)
        {
            o.vx += (Hash01(o.seed + 20.0f) - 0.5f) * shock * 0.03f;
            o.vy += (Hash01(o.seed + 21.0f) - 0.5f) * shock * 0.03f;
            o.vz += (Hash01(o.seed + 22.0f) - 0.5f) * shock * 0.02f;
        }

        o.vx *= 0.985f;
        o.vy *= 0.985f;
        o.vz *= 0.985f;

        o.x += o.vx;
        o.y += o.vy;
        o.z += o.vz;

        o.vrx += smBass * 0.0022f + n1 * 0.0008f;
        o.vry += smMid * 0.0022f + n2 * 0.0008f;
        o.vrz += smHigh * 0.0022f + n3 * 0.0008f;

        if (shock > 0.25f)
        {
            o.vrx += shock * 0.03f;
            o.vry += shock * 0.025f;
            o.vrz += shock * 0.02f;
        }

        o.vrx *= 0.98f;
        o.vry *= 0.98f;
        o.vrz *= 0.98f;

        o.rx += o.vrx;
        o.ry += o.vry;
        o.rz += o.vrz;

        if (o.x < -1.2f || o.x > 1.2f) o.vx *= -0.85f;
        if (o.y < -0.9f || o.y > 0.9f) o.vy *= -0.85f;
        if (o.z < 0.0f || o.z > 1.4f) o.vz *= -0.85f;
    }

    for (std::size_t i = 0; i < objects.size(); ++i)
        for (std::size_t j = i + 1; j < objects.size(); ++j)
            ApplyCollision(objects[i], objects[j]);

    const float bgR = 0.03f + smBass * 0.18f + smEnergy * 0.05f;
    const float bgG = 0.03f + smMid * 0.10f;
    const float bgB = 0.05f + smHigh * 0.20f + smEnergy * 0.08f;
    glClearColor(std::clamp(bgR, 0.0f, 1.0f), std::clamp(bgG, 0.0f, 1.0f), std::clamp(bgB, 0.0f, 1.0f), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glPushMatrix();
    glTranslatef(shakeX, shakeY, 0.0f);

    const float lavaDrift = t * (0.035f + smEnergy * 0.045f);
    for (int i = 0; i < 6; ++i)
    {
        const float seed = static_cast<float>(frameIndex) * 19.7f + static_cast<float>(i) * 9.4f;
        const float phase = lavaDrift + static_cast<float>(i) * 0.91f;
        const float wobble = std::sin(phase * 1.7f + seed) * 0.10f;
        const float wobble2 = std::cos(phase * 1.2f + seed * 0.3f) * 0.07f;

        const float x = std::sin(phase + smMid * 2.6f) * (0.42f + smBass * 0.12f) + wobble;
        const float y = std::cos(phase * 0.83f + smBass * 1.7f) * (0.28f + smEnergy * 0.07f) + wobble2;
        const float r = 0.16f + smBass * 0.16f + Hash01(seed) * 0.11f;
        const float g = 0.10f + smMid * 0.18f + Hash01(seed + 1.0f) * 0.12f;
        const float b = 0.22f + smHigh * 0.22f + Hash01(seed + 2.0f) * 0.16f;
        const float alpha = 0.18f + smEnergy * 0.22f;

        DrawBlob(x, y, r + smBass * 0.20f, r, g, b, alpha, 34);
    }

    const int sphereCount = 4 + static_cast<int>(smEnergy * 4.0f);
    for (int i = 0; i < sphereCount; ++i)
    {
        const float seed = static_cast<float>(frameIndex) * 27.3f + static_cast<float>(i) * 5.7f;
        const float x = -0.65f + Hash01(seed) * 1.30f;
        const float y = -0.45f + Hash01(seed + 0.7f) * 0.90f;
        const float size = 0.08f + Hash01(seed + 2.4f) * 0.16f + smBass * 0.08f;
        const float rot = t * (0.4f + i * 0.15f) + Hash01(seed + 4.3f) * 6.0f;

        DrawChromaticSplit(x, y, size, rot, smBass, smMid, smHigh, glitch);
    }

    for (const auto& o : objects)
    {
        const float depth = 1.0f / (1.0f + o.z * 1.5f);
        const float px = o.x * depth;
        const float py = o.y * depth;
        const float psize = o.size * depth * (1.0f + smBass * 0.4f);

        const float r = 0.30f + smBass * 0.70f;
        const float g = 0.30f + smMid * 0.70f;
        const float b = 0.40f + smHigh * 0.80f;
        const float a = 0.20f + depth * 0.45f;

        if (o.type == 0)
        {
            DrawSphereLite(px, py, psize, o.rx, o.ry, o.rz, r, g, b, a);
        }
        else if (o.type == 1)
        {
            DrawConeIso(px, py, psize, o.ry, o.rx, o.rz, r, g, b, a);
        }
        else
        {
            DrawCubeIso(px, py, psize, o.ry, o.rx, o.rz, r, g, b, a);
        }

        DrawHexRing(px, py, psize * 0.95f, o.rz, 0.95f, 0.85f, 0.95f, 0.08f + glitch * 0.06f);
    }

    DrawScanlines(t, smHigh, glitch);
    DrawVhsBars(t, smHigh, smEnergy, glitch);
    DrawNoise(static_cast<float>(frameIndex) + t * 0.05f, smHigh, smEnergy, glitch);

    const float pulse = 0.04f + beatPulse * 0.14f + smEnergy * 0.05f;
    DrawBlob(0.0f, 0.0f, pulse, 1.0f, 0.9f, 0.55f, 0.72f);

    const float screenWarp = 0.012f + smHigh * 0.04f + glitch * 0.02f;
    for (int i = 0; i < 10; ++i)
    {
        const float y = -1.0f + (static_cast<float>(i) / 9.0f) * 2.0f;
        const float offset = std::sin(y * 7.0f + t * 2.0f + smHigh * 8.0f) * screenWarp;
        DrawLine(-1.0f + offset, y, 1.0f + offset, y, 0.05f, 0.06f, 0.08f, 0.08f + glitch * 0.03f);
    }

    const float barProgress = dragging ? Clamp01(scrubProgress) : Clamp01(songProgress);
    DrawQuad(-1.0f, -0.95f, 1.0f, -0.90f, 0.06f, 0.06f, 0.08f, 1.0f);
    DrawQuad(-1.0f, -0.95f, -1.0f + 2.0f * barProgress, -0.90f, 0.20f + smBass * 0.5f, 0.75f, 0.25f + smHigh * 0.4f, 1.0f);

    const float handleX = -1.0f + 2.0f * barProgress;
    DrawQuad(handleX - 0.010f, -0.97f, handleX + 0.010f, -0.88f, 1.0f, 0.6f, 0.2f, 1.0f);

    glPopMatrix();
}