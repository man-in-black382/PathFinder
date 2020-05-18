#pragma once

#include <cstdint>

namespace PathFinder 
{

    struct GTTonemappingParameterss
    {
        float MaximumLuminance = 270.0; // Your typical SDR monitor max luminance
        float Contrast = 1.0;
        float LinearSectionStart = 0.22;
        float LinearSectionLength = 0.4;
        // 16 byte boundary
        float BlackTightness = 1.33;
        float MinimumBrightness = 0.0;
        uint32_t __Pad0;
        uint32_t __Pad1;
    };

}
