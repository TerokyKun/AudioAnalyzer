#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "AudioEngine.h"
#include "FFT.h"

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

int main()
{
    // --- Инициализация GLFW
    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Audio Visualizer", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGL();

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    // --- Загрузка аудио
    AudioEngine engine;
    if (!engine.Load("audio/track1.mp3")) return -1;

    std::vector<float> spectrum;

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // --- Получаем текущее окно сэмплов (например, 1024 последних)
        std::vector<float> windowSamples(engine.GetSamples().begin(), engine.GetSamples().begin() + 1024);
        FFT::Compute(windowSamples, spectrum);

        // --- Отрисовка спектра
        glBegin(GL_LINES);
        for (size_t i = 0; i < spectrum.size(); i++)
        {
            float x = (float)i / spectrum.size() * 2.0f - 1.0f;
            float y = spectrum[i] / 50.0f; // нормировка
            glColor3f(1.0f - y, y, 0.5f);
            glVertex2f(x, -1.0f);
            glVertex2f(x, -1.0f + y);
        }
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}