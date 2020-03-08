#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace PathFinder
{

    struct PerFrameRootConstants
    {
        glm::vec4 CameraPosition;
        glm::mat4 CameraView;
        glm::mat4 CameraProjection;
        glm::mat4 CameraViewProjection;
        glm::mat4 CameraInverseView;
        glm::mat4 CameraInverseProjection;
        glm::mat4 CameraInverseViewProjection;
        float CameraExposureValue100;
    };

}
