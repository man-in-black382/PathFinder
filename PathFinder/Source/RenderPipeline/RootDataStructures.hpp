#pragma once

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

namespace PathFinder
{

    struct RootGlobalData
    {
        glm::ivec2 BackBufferResolution;
    };

    struct RootPerFrameData
    {
        glm::mat4 CameraPosition;
        glm::mat4 CameraView;
        glm::mat4 CameraProjection;
        glm::mat4 CameraViewProjection;
        glm::mat4 CameraInverseView;
        glm::mat4 CameraInverseProjection;
        glm::mat4 CameraInverseViewProjection;
    };

}
