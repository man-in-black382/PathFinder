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
            ResourceNames::StochasticShadowedShadingOutput[frameIndex];
    }

    Foundation::Name DenoiserPostBlurStochasticUnshadowedInputTexName(bool isDenoiserEnabled, uint64_t frameIndex)
    {
        return isDenoiserEnabled ?
            ResourceNames::StochasticUnshadowedShadingDenoised[frameIndex] :
            ResourceNames::StochasticUnshadowedShadingOutput;
    }

    Foundation::Name DeferredLightingRngSeedTexName(bool isDenoiserEnabled, uint64_t frameIndex)
    {
        return isDenoiserEnabled ?
            ResourceNames::RngSeedsCorrelated : 
            ResourceNames::RngSeeds[frameIndex];
    }

    Foundation::Name DenoiserHistoryFixShadowedInputTexName(bool isDenoiserPreBlurEnabled, uint64_t frameIndex)
    {
        return isDenoiserPreBlurEnabled ? 
            ResourceNames::StochasticShadowedShadingPreBlurred : 
            ResourceNames::StochasticShadowedShadingOutput[frameIndex];
    }

    Foundation::Name DenoiserHistoryFixUnshadowedInputTexName(bool isDenoiserPreBlurEnabled)
    {
        return isDenoiserPreBlurEnabled ?
            ResourceNames::StochasticUnshadowedShadingPreBlurred : 
            ResourceNames::StochasticUnshadowedShadingOutput;
    }

    Foundation::Name DenoiserGradientConstructionShadowedInputTexName(bool isDenoiserPreBlurEnabled, uint64_t frameIndex)
    {
        return isDenoiserPreBlurEnabled ?
            ResourceNames::StochasticShadowedShadingPreBlurred :
            ResourceNames::StochasticShadowedShadingOutput[frameIndex];
    }

    Foundation::Name DenoiserGradientConstructionUnshadowedInputTexName(bool isDenoiserPreBlurEnabled)
    {
        return isDenoiserPreBlurEnabled ?
            ResourceNames::StochasticUnshadowedShadingPreBlurred : 
            ResourceNames::StochasticUnshadowedShadingOutput;
    }

    Foundation::Name DenoiserMipGenerationShadowedInputTexName(bool isDenoiserPreBlurEnabled, uint64_t frameIndex)
    {
        return isDenoiserPreBlurEnabled ?
            ResourceNames::StochasticShadowedShadingPreBlurred :
            ResourceNames::StochasticShadowedShadingOutput[frameIndex];
    }

    Foundation::Name DenoiserMipGenerationUnshadowedInputTexName(bool isDenoiserPreBlurEnabled)
    {
        return isDenoiserPreBlurEnabled ?
            ResourceNames::StochasticUnshadowedShadingPreBlurred :
            ResourceNames::StochasticUnshadowedShadingOutput;
    }

}
