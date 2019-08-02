#include "RenderContext.hpp"

namespace PathFinder
{

    RenderContext::RenderContext(const Scene* scene, IGraphicsDevice* graphicsDevice)
        : mScene{ scene }, mGraphicsDevice{ graphicsDevice } {}

}
