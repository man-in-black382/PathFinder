#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <Scene/SceneGPUTypes.hpp>

namespace PathFinder
{

    struct PerFrameRootConstants
    {
        GPUCamera CurrentFrameCamera;
        GPUCamera PreviousFrameCamera;
        // Cameras are 16 byte aligned
        glm::uvec2 MousePosition;
        uint32_t IsDenoiserEnabled;
        uint32_t IsReprojectionHistoryDebugEnabled;
        // 16 Byte Boundary
        uint32_t IsGradientDebugEnabled;
        uint32_t IsMotionDebugEnabled;
        uint32_t IsDenoiserAntilagEnabled;
        uint32_t IsSMAAEnabled;
        // 16 Byte Boundary
        uint32_t IsSMAAEdgeDetectionEnabled;
        uint32_t IsSMAABlendingWeightCalculationEnabled;
        uint32_t IsSMAANeighborhoodBlendingEnabled;
        uint32_t IsTAAEnabled;
        // 16 Byte Boundary
        uint32_t IsTAAYCoCgSpaceEnabled;
        uint32_t Pad0__;
        uint32_t Pad1__;
        uint32_t Pad2__;
    };

}
