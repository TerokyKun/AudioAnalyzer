#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

#include "Analyzer.h"
#include "AudioEngine.h"
#include "Visualizer.h"

struct AppState
{
    AudioEngine* audio = nullptr;
    Analyzer* analyzer = nullptr;
    Visualizer* visualizer = nullptr;

    int width = 1280;
    int height = 720;

    bool dragging = false;
    float dragProgress = 0.0f;
    float beatPulse = 0.0f;
    std::size_t frameIndex = 0;
};

static void Setup2D(int width, int height)
{
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

static void UpdateDragFromCursor(GLFWwindow* window)
{
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!state)
        return;

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    if (state->width <= 0)
        return;

    const float px = static_cast<float>(x) / static_cast<float>(state->width);
    state->dragProgress = std::clamp(px, 0.0f, 1.0f);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int /*mods*/)
{
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!state || !state->audio)
        return;

    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(window, &x, &y);

    const float fy = static_cast<float>(y) / std::max(1, state->height);
    const bool onSeekBar = fy >= 0.90f;

    if (action == GLFW_PRESS && onSeekBar)
    {
        state->dragging = true;
        state->audio->Pause();
        UpdateDragFromCursor(window);
    }
    else if (action == GLFW_RELEASE && state->dragging)
    {
        state->dragging = false;

        const double targetSec = state->dragProgress * state->audio->GetDurationSec();
        state->audio->SeekToSec(targetSec);
        state->audio->Resume();
    }
}

static void CursorPosCallback(GLFWwindow* window, double /*xpos*/, double /*ypos*/)
{
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (!state)
        return;

    if (state->dragging)
        UpdateDragFromCursor(window);
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    auto* state = static_cast<AppState*>(glfwGetWindowUserPointer(window));
    if (state)
    {
        state->width = width;
        state->height = height;
    }

    Setup2D(width, height);
}

static void DrawCenteredLoading(const Visualizer& /*visualizer*/, float progress)
{
    const float p = std::clamp(progress, 0.0f, 1.0f);

    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_QUADS);
    glColor4f(0.08f, 0.08f, 0.10f, 1.0f);
    glVertex2f(-1.0f, -0.95f);
    glVertex2f(1.0f, -0.95f);
    glVertex2f(1.0f, -0.90f);
    glVertex2f(-1.0f, -0.90f);
    glEnd();

    glBegin(GL_QUADS);
    glColor4f(0.20f, 0.80f, 0.30f, 1.0f);
    glVertex2f(-1.0f, -0.95f);
    glVertex2f(-1.0f + 2.0f * p, -0.95f);
    glVertex2f(-1.0f + 2.0f * p, -0.90f);
    glVertex2f(-1.0f, -0.90f);
    glEnd();

    const float pulse = 0.18f + p * 0.42f;
    glPushMatrix();
    glRotatef(p * 120.0f, 0.0f, 0.0f, 1.0f);
    glScalef(pulse, pulse, 1.0f);
    glBegin(GL_QUADS);
    glColor4f(0.18f + p * 0.5f, 0.25f + p * 0.4f, 0.65f + p * 0.2f, 0.92f);
    glVertex2f(-0.5f, -0.5f);
    glVertex2f(0.5f, -0.5f);
    glVertex2f(0.5f, 0.5f);
    glVertex2f(-0.5f, 0.5f);
    glEnd();
    glPopMatrix();
}

int main()
{
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "AudioVisualizer", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL())
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    int width = 1280;
    int height = 720;
    glfwGetFramebufferSize(window, &width, &height);
    Setup2D(width, height);

    std::filesystem::create_directories("analysis");

    const std::filesystem::path audioPath = "audio/track1.mp3";
    const std::filesystem::path jsonPath = std::filesystem::path("analysis") / (audioPath.stem().string() + ".analysis.json");

    AudioEngine audio;
    if (!audio.Load(audioPath.string()))
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    Analyzer analyzer;
    Visualizer visualizer;

    AppState state;
    state.audio = &audio;
    state.analyzer = &analyzer;
    state.visualizer = &visualizer;
    state.width = width;
    state.height = height;

    glfwSetWindowUserPointer(window, &state);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    std::atomic<bool> analysisDone{false};

    std::thread analysisThread([&]()
    {
        analyzer.Process(audio.GetSamples(), static_cast<int>(audio.GetSampleRate()), static_cast<int>(audio.GetChannels()));
        analyzer.SaveJSON(jsonPath.string());
        analysisDone.store(true, std::memory_order_relaxed);
    });

    while (!analysisDone.load(std::memory_order_relaxed) && !glfwWindowShouldClose(window))
    {
        const float progress = analyzer.GetProgress();

        char title[128];
        std::snprintf(title, sizeof(title), "Analyzing... %3.0f%%", progress * 100.0f);
        glfwSetWindowTitle(window, title);

        DrawCenteredLoading(visualizer, progress);

        glfwSwapBuffers(window);
        glfwPollEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    analysisThread.join();

    audio.Play();

    const auto& frames = analyzer.GetFrames();
    std::size_t index = 0;

    double lastTime = glfwGetTime();
    state.beatPulse = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        const double now = glfwGetTime();
        const float dt = static_cast<float>(now - lastTime);
        lastTime = now;

        const float t = state.dragging
            ? state.dragProgress * static_cast<float>(audio.GetDurationSec())
            : audio.GetPlaybackTimeSec();

        while (index + 1 < frames.size() && frames[index + 1].t <= t)
            ++index;

        if (!frames.empty() && index >= frames.size())
            index = frames.size() - 1;

        const FrameData frame = frames.empty() ? FrameData{} : frames[index];

        if (frame.beat)
            state.beatPulse = 1.0f;

        state.beatPulse = std::max(0.0f, state.beatPulse - dt * 2.5f);

        const float songProgress = audio.GetDurationSec() > 0.0
            ? std::min(1.0f, t / static_cast<float>(audio.GetDurationSec()))
            : 0.0f;

        glClearColor(
            std::clamp(0.03f + frame.energy * 0.22f, 0.0f, 1.0f),
            std::clamp(0.03f + frame.mid * 0.08f, 0.0f, 1.0f),
            std::clamp(0.05f + frame.high * 0.20f, 0.0f, 1.0f),
            1.0f
        );
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        visualizer.DrawFrame(frame, t, songProgress, state.beatPulse, state.dragging, state.dragProgress, index);

        glfwSwapBuffers(window);
        glfwPollEvents();

        char title[256];
        if (state.dragging)
            std::snprintf(title, sizeof(title), "Scrubbing... %3.0f%%  |  t=%.2fs", state.dragProgress * 100.0f, state.dragProgress * static_cast<float>(audio.GetDurationSec()));
        else
            std::snprintf(title, sizeof(title), "Playing... %3.0f%%  |  t=%.2fs", songProgress * 100.0f, t);

        glfwSetWindowTitle(window, title);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    audio.Stop();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}