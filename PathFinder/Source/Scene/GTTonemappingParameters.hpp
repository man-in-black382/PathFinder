#pragma once

namespace PathFinder 
{

    struct GTTonemappingParams
    {
        float MaximumLuminance = 270.0; // Your typical SDR monitor max luminance
        float Contrast = 1.0;
        float LinearSectionStart = 0.22;
        float LinearSectionLength = 0.4;
        float BlackTightness = 1.33;
        float MinimumBrightness = 0.0;
    };

}
