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
        const HAL::TextureProperties& textureProperties = resourceProvider.GetTextureProperties(textureName);

        int64_t mipCount = textureProperties.MipCount;
        const Geometry::Dimensions& textureDimensions = textureProperties.Dimensions;

        if (maxMipLevel)
        {
            mipCount = std::min(int64_t(*maxMipLevel) + 1, mipCount);
        }

        std::vector<DownsamplingInvocationInputs> inputsArray;

        constexpr int64_t DownsampleLevelsProducedByOneInvocation = 4;

        for (int64_t mipLevel = 0, localIndex = 0; mipLevel < mipCount - 1; ++mipLevel)
        {
            DownsamplingInvocationInputs* inputs = localIndex == 0 ?
                &inputsArray.emplace_back() : &inputsArray.back();

            Geometry::Dimensions mipDimensions = textureProperties.MipSize(mipLevel);

            if (localIndex == 0)
            {
                bool isSourceSRV = mipLevel == 0;

                inputs->DispatchDimensions = textureDimensions.XYMultiplied(1.0 / pow(2.0, mipLevel + 1));
                inputs->CBContent.InputSize = { mipDimensions.Width, mipDimensions.Height };
                inputs->CBContent.IsSourceTexSRV = isSourceSRV;
                inputs->CBContent.FilterType = filter;
                inputs->CBContent.SourceTexIdx = isSourceSRV ?
                    resourceProvider.GetSRTextureIndex(textureName, mipLevel) :
                    resourceProvider.GetUATextureIndex(textureName, mipLevel);
            }

            inputs->CBContent.NumberOfOutputsToCompute = localIndex + 1;
            inputs->CBContent.OutputsToWrite[localIndex] = strategy == DownsamplingStrategy::WriteAllLevels;
            inputs->CBContent.OutputTexIndices[localIndex] = resourceProvider.GetUATextureIndex(textureName, mipLevel + 1);

            auto lastLocalIndex = localIndex;
            localIndex = (localIndex + 1) % (DownsampleLevelsProducedByOneInvocation - 1);

            // Check if the height or width of the next generated mip is odd
            // For odd dimensions, create a new dispatch so that undersampling doesn't occur
            // If offsets have to be calculated while swizzling threads for odd width/height,
            // we will have to read thread values from neighboring threadgroups as well,
            // which will lead to sync barriers which we want to avoid and so we have a new dispatch
            // for odd sized mips.
            inputs->CBContent.IsInputSizeOddHorizontally = (mipDimensions.Width & 1) != 0;
            inputs->CBContent.IsInputSizeOddVertically = (mipDimensions.Height & 1) != 0;

            if (inputs->CBContent.IsInputSizeOddHorizontally || inputs->CBContent.IsInputSizeOddVertically)
            {
                localIndex = 0;
            }

            if (localIndex == 0 && strategy == DownsamplingStrategy::WriteOnlyLastLevel)
            {
                inputs->CBContent.OutputsToWrite[lastLocalIndex] = true;
            }
        }

        return inputsArray;
    }

}
