#pragma once

#include <cstddef>
#include "Analyzer.h"

class Visualizer
{
public:
    void Draw(float bass, float mid, float high, bool beat);
    void DrawProgress(float progress);

    void DrawLoading(float progress);
    void DrawFrame(const FrameData& frame, float playbackSec, float songProgress, float beatPulse, bool dragging, float scrubProgress, std::size_t frameIndex);
};