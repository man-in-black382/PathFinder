#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <Scene/SceneGPUStorage.hpp>

namespace PathFinder
{

    struct PerFrameRootConstants
    {
        GPUCamera CurrentFrameCamera;
        GPUCamera PreviousFrameCamera;
        // Cameras are 16 byte aligned
        uint32_t IsDenoiserEnabled;
        uint32_t IsReprojectionHistoryDebugEnabled;
        uint32_t IsGradientDebugEnabled;
        uint32_t IsMotionDebugEnabled;
        uint32_t IsDenoiserAntilagEnabled;
    };

}
