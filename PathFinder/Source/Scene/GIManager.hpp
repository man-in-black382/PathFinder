#pragma once

#include "Camera.hpp"

#include <glm/vec3.hpp>

namespace PathFinder
{

    class GIManager
    {
    public:
        void SetCamera(const Camera* camera);
        void SetProbeGridSize(const glm::uvec3& size);

    private:
        const Camera* mCamera = nullptr;
        glm::uvec3 mProbeGridSize;

    public:
        
    };

}
