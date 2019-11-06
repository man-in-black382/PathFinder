#include "RenderContext.hpp"

namespace PathFinder
{

    RenderContext::RenderContext(const Scene* scene, GraphicCommandRecorder* graphicCommandRecorder, 
        RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider,
        const RenderSurfaceDescription& defaultRenderSurface)
        : 
        mScene{ scene }, mGrapicCommandRecorder{ graphicCommandRecorder },
        mRootConstantsUpdater{ rootConstantsUpdater }, mResourceProvider{ resourceProvider },
        mDefaultRenderSurface{ defaultRenderSurface } {}

}
