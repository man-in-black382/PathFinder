#include "DownsamplingHelper.hpp"

#include <algorithm>
#include <cmath>

namespace PathFinder
{
   
    std::vector<DownsamplingInvocationInputs> GenerateDownsamplingShaderInvocationInputs(
        Foundation::Name textureName,
        const HAL::TextureProperties& textureProperties,
        DownsamplingCBContent::Filter filter, 
        DownsamplingStrategy strategy,
        std::optional<uint64_t> maxMipLevel)
    {
        int64_t mipCount = textureProperties.MipCount;
        const Geometry::Dimensions& textureDimensions = textureProperties.Dimensions;

        if (maxMipLevel)
        {
            mipCount = std::min(int64_t(*maxMipLevel) + 1, mipCount);
        }

        std::vector<DownsamplingInvocationInputs> inputsArray;

        for (int64_t mipLevel = 0, localIndex = 0; mipLevel < mipCount - 1; ++mipLevel)
        {
            DownsamplingInvocationInputs* inputs = nullptr;
            
            if (localIndex == 0)
            {
                inputs = &inputsArray.emplace_back();
                inputs->CBContent.InvocationIdx = inputsArray.size() - 1;
            }
            else
            {
                inputs = &inputsArray.back();
            }

            inputs->ResourceName = textureName;
            Geometry::Dimensions mipDimensions = textureProperties.MipSize(mipLevel);

            if (localIndex == 0)
            {
                inputs->DispatchDimensions = textureDimensions.XYMultiplied(1.0 / pow(2.0, mipLevel + 1));
                inputs->CBContent.DispatchDimensions = { inputs->DispatchDimensions.Width, inputs->DispatchDimensions.Height };
                inputs->CBContent.DispatchDimensionsInv = { 1.0 / inputs->DispatchDimensions.Width, 1.0 / inputs->DispatchDimensions.Height };
                inputs->CBContent.SourceDimensions = { mipDimensions.Width, mipDimensions.Height };
                inputs->CBContent.SourceDimensionsInv = { 1.0 / mipDimensions.Width, 1.0 / mipDimensions.Height };
                inputs->CBContent.FilterType = filter;
                inputs->CBContent.SourceMipIdx = mipLevel;
                inputs->SourceMip = mipLevel;
            }

            inputs->CBContent.NumberOfOutputsToCompute = localIndex + 1;
            inputs->CBContent.OutputsToWrite[localIndex] = strategy == DownsamplingStrategy::WriteAllLevels;

            auto previousLocalIndex = localIndex;
            localIndex = (localIndex + 1) % (DownsamplingInvocationInputs::MaxMipsProcessedByInvocation - 1);

            // Check if the height or width of the next generated mip is odd
            // For odd dimensions, create a new dispatch so that undersampling doesn't occur
            // If offsets have to be calculated while swizzling threads for odd width/height,
            // we will have to read thread values from neighboring threadgroups as well,
            // which will lead to sync barriers which we want to avoid and so we have a new dispatch
            // for odd sized mips.
            inputs->CBContent.IsInputSizeOddHorizontally = (mipDimensions.Width & 1) != 0;
            inputs->CBContent.IsInputSizeOddVertically = (mipDimensions.Height & 1) != 0;

            bool anyDimensionOdd = inputs->CBContent.IsInputSizeOddHorizontally || inputs->CBContent.IsInputSizeOddVertically;

            if (anyDimensionOdd)
            {
                // Trigger new invocation if any dimension is odd
                localIndex = 0;
            }

            if (localIndex == 0 && strategy == DownsamplingStrategy::WriteOnlyLastLevel)
            {
                inputs->CBContent.OutputsToWrite[previousLocalIndex] = true;
            }
        }

        return inputsArray;
    }

    void UpdateDownsamplingInputsWithTextureIndices(DownsamplingInvocationInputs& inputs, const ResourceProvider* resourceProvider)
    {
        inputs.CBContent.SourceTexIdx = resourceProvider->GetSRTextureIndex(inputs.ResourceName, inputs.SourceMip);

        for (auto outputIdx = 0; outputIdx < DownsamplingInvocationInputs::MaxMipsProcessedByInvocation; ++outputIdx)
        {
            auto nextMip = inputs.SourceMip + outputIdx + 1;

            if (inputs.CBContent.OutputsToWrite[outputIdx])
            {
                inputs.CBContent.OutputTexIndices[outputIdx] = resourceProvider->GetUATextureIndex(inputs.ResourceName, nextMip);
            }
        }
    }

}
