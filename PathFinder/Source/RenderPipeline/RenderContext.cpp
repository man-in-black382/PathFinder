#include "RenderContext.hpp"

namespace PathFinder
{

    RenderContext::RenderContext(
        const Scene* scene,
        const MeshGPUStorage* meshStorage,
        GPUCommandRecorder* graphicCommandRecorder,
        RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider,
        const RenderSurfaceDescription& defaultRenderSurface)
        : 
        mScene{ scene }, 
        mMeshStorage{ meshStorage },
        mGrapicCommandRecorder{ graphicCommandRecorder },
        mRootConstantsUpdater{ rootConstantsUpdater },
        mResourceProvider{ resourceProvider },
        mDefaultRenderSurface{ defaultRenderSurface }
    {}

}
