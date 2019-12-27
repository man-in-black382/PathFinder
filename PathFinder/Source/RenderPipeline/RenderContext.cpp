#include "RenderContext.hpp"

namespace PathFinder
{

    RenderContext::RenderContext(
        const Scene* scene,
        const MeshGPUStorage* meshStorage,
        const UIGPUStorage* uiStorage,
        GPUCommandRecorder* graphicCommandRecorder,
        RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider,
        const RenderSurfaceDescription& defaultRenderSurface)
        : 
        mScene{ scene }, 
        mMeshStorage{ meshStorage },
        mUIStorage{ uiStorage },
        mGrapicCommandRecorder{ graphicCommandRecorder },
        mRootConstantsUpdater{ rootConstantsUpdater },
        mResourceProvider{ resourceProvider },
        mDefaultRenderSurface{ defaultRenderSurface }
    {}

}
