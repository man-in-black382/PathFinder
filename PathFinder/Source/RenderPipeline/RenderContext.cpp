#include "RenderContext.hpp"

namespace PathFinder
{

    RenderContext::RenderContext(
        const Scene* scene,
        const AssetResourceStorage* assetStorage,
        const VertexStorage* vertexStorage, 
        GPUCommandRecorder* graphicCommandRecorder,
        RootConstantsUpdater* rootConstantsUpdater, ResourceProvider* resourceProvider,
        const RenderSurfaceDescription& defaultRenderSurface)
        : 
        mScene{ scene }, 
        mAssetStorage{ assetStorage },
        mVertexStorage{ vertexStorage },
        mGrapicCommandRecorder{ graphicCommandRecorder },
        mRootConstantsUpdater{ rootConstantsUpdater },
        mResourceProvider{ resourceProvider },
        mDefaultRenderSurface{ defaultRenderSurface }
    {}

}
