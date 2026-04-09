#pragma once
#include <vector>
#include <string>

class Ananlyzer
{
    public:
    void Process(count std::vector<float>& saples, int sampleRate);
    void SaveJSON(const std::string& path);

    private:
    struct Frame
    {
        float t; 
        float bass;
        float mid;
        float high;
        bool beat;
    };
    std::vector<frame> frames;
}