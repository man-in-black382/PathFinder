#include "ResourceNameResolving.hpp"

namespace PathFinder
{

    Foundation::Name SMAAEdgeDetectionPassInputTexName(bool isGIDebugEnabled)
    {
        return isGIDebugEnabled ? ResourceNames::GIDebugOutput : ResourceNames::BloomCompositionOutput;
    }

    Foundation::Name DenoiserPostBlurStochasticShadowedInputTexName(bool isDenoiserEnabled, uint64_t frameIndex)
    {
        return isDenoiserEnabled ?
            ResourceNames::StochasticShadowedShadingDenoised[frameIndex] : 
            ResourceNames::StochasticShadowedShadingOutput;
    }

    Foundation::Name DenoiserPostBlurStochasticUnshadowedInputTexName(bool isDenoiserEnabled, uint64_t frameIndex)
    {
        return isDenoiserEnabled ?
            ResourceNames::StochasticUnshadowedShadingDenoised[frameIndex] :
            ResourceNames::StochasticUnshadowedShadingOutput;
    }

    Foundation::Name DeferredLightingRngSeedTexName(bool isDenoiserEnabled, uint64_t frameIndex)
    {
        return isDenoiserEnabled ? ResourceNames::RngSeedsCorrelated : ResourceNames::RngSeeds[frameIndex];
    }

}
