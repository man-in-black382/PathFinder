#pragma once

#include "PipelineNames.hpp"

namespace PathFinder
{

    // Here we define helpers that will resolve resource names for situations when certain render passes are disabled
    // and resource read/write chain needs to be reconfigured. We should do it here to avoid performing checks all over the engine.

    Foundation::Name SMAAEdgeDetectionPassInputTexName(bool isGIDebugEnabled);
    Foundation::Name DenoiserPostBlurStochasticShadowedInputTexName(bool isDenoiserEnabled, uint64_t frameIndex);
    Foundation::Name DenoiserPostBlurStochasticUnshadowedInputTexName(bool isDenoiserEnabled, uint64_t frameIndex);
    Foundation::Name DeferredLightingRngSeedTexName(bool isDenoiserEnabled, uint64_t frameIndex);
    Foundation::Name DenoiserHistoryFixShadowedInputTexName(bool isDenoiserPreBlurEnabled, uint64_t frameIndex);
    Foundation::Name DenoiserHistoryFixUnshadowedInputTexName(bool isDenoiserPreBlurEnabled);
    Foundation::Name DenoiserGradientConstructionShadowedInputTexName(bool isDenoiserPreBlurEnabled, uint64_t frameIndex);
    Foundation::Name DenoiserGradientConstructionUnshadowedInputTexName(bool isDenoiserPreBlurEnabled);
    Foundation::Name DenoiserMipGenerationShadowedInputTexName(bool isDenoiserPreBlurEnabled, uint64_t frameIndex);
    Foundation::Name DenoiserMipGenerationUnshadowedInputTexName(bool isDenoiserPreBlurEnabled);

}
