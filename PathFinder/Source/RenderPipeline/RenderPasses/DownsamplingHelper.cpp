#include "DownsamplingHelper.hpp"

#include <algorithm>
#include <cmath>

namespace PathFinder
{
   
    std::vector<DownsamplingInvocationInputs> GenerateDownsamplingShaderInvocationInputs(
        const ResourceProvider& resourceProvider,
        Foundation::Name textureName,
        DownsamplingCBContent::Filter filter, 
        DownsamplingStrategy strategy,
        std::optional<uint64_t> maxMipLevel)
    {
        const HAL::Texture::Properties& textureProperties = resourceProvider.GetTextureProperties(textureName);

        int64_t mipCount = textureProperties.MipCount;
        const Geometry::Dimensions& textureDimensions = textureProperties.Dimensions;

        if (maxMipLevel)
        {
            mipCount = std::min(int64_t(*maxMipLevel) + 1, mipCount);
        }

        std::vector<DownsamplingInvocationInputs> inputsArray;

        constexpr int64_t DownsampleLevelsProducedByOneInvocation = 4;
        int64_t invocationCount = std::ceil(float(mipCount - 1) / DownsampleLevelsProducedByOneInvocation);
        int64_t mipLevel = 0;

        for (auto invocationIdx = 0ll; invocationIdx < invocationCount; ++invocationIdx)
        {
            int64_t mipsLeftToProcess = std::max(0ll, mipCount - invocationIdx * DownsampleLevelsProducedByOneInvocation - 1);

            DownsamplingInvocationInputs& inputs = inputsArray.emplace_back();
            DownsamplingCBContent& cbContent = inputs.CBContent;

            inputs.DispatchDimensions = textureDimensions.XYMultiplied(1.0 / pow(2.0, mipLevel + 1));

            bool isSourceSRV = invocationIdx == 0;
            cbContent.IsSourceTexSRV = isSourceSRV;

            cbContent.SourceTexIdx = isSourceSRV ?
                resourceProvider.GetSRTextureIndex(textureName, mipLevel) :
                resourceProvider.GetUATextureIndex(textureName, mipLevel);

            cbContent.FilterType = filter;
            cbContent.NumberOfOutputsToCompute = std::min(mipsLeftToProcess, DownsampleLevelsProducedByOneInvocation);
            
            auto localMipIndex = 0u;

            for (; localMipIndex < cbContent.NumberOfOutputsToCompute; ++localMipIndex)
            {
                ++mipLevel;
                cbContent.OutputsToWrite[localMipIndex] = strategy == DownsamplingStrategy::WriteAllLevels;
                cbContent.OutputTexIndices[localMipIndex] = resourceProvider.GetUATextureIndex(textureName, mipLevel);
            }

            if (strategy == DownsamplingStrategy::WriteOnlyLastLevel)
            {
                cbContent.OutputsToWrite[localMipIndex - 1] = true;
            }
        }

        return inputsArray;
    }

}
