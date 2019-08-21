#include "RenderContext.hpp"

namespace PathFinder
{

    RenderContext::RenderContext(const Scene* scene, IGraphicsDevice* graphicsDevice, RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider)
        : mScene{ scene }, mGraphicsDevice{ graphicsDevice }, mRootConstantsUpdater{ rootConstantsUpdater }, mResourceProvider{ resourceProvider } {}

}
