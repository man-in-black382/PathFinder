#include "GIManager.hpp"

namespace PathFinder 
{

    void GIManager::SetCamera(const Camera* camera)
    {
        mCamera = camera;
    }

    void GIManager::SetProbeGridSize(const glm::uvec3& size)
    {
        mProbeGridSize = size;
    }

}
