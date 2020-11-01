#pragma once

#include <cstdint>
#include <vector>
#include <optional>

#include <glm/vec4.hpp>

#include <Geometry/Dimensions.hpp>

#include "../RenderPassMediators/ResourceProvider.hpp"

namespace PathFinder
{

    struct DownsamplingCBContent
    {
        enum class Filter : uint32_t
        { 
            Average = 0, Min = 1, Max = 2, MinMaxLuminance = 3
        };

        Filter FilterType = Filter::Average;
        uint32_t SourceTexIdx = 0;
        uint32_t SourceMipIdx = 0;
        uint32_t NumberOfOutputsToCompute = 0;
        glm::uvec4 OutputTexIndices;
        glm::uvec4 OutputsToWrite;
        glm::vec2 DispatchDimensions;
        glm::vec2 DispatchDimensionsInv;
        glm::vec2 SourceDimensions;
        glm::vec2 SourceDimensionsInv;
        uint32_t IsInputSizeOddVertically = false;
        uint32_t IsInputSizeOddHorizontally = false;
        uint32_t InvocationIdx = 0;
    };

    enum class DownsamplingStrategy
    {
        WriteAllLevels, WriteOnlyLastLevel
    };

    struct DownsamplingInvocationInputs
    {
        static const uint8_t MaxMipsProcessedByInvocation = 4;

        DownsamplingCBContent CBContent;
        Geometry::Dimensions DispatchDimensions;
        Foundation::Name ResourceName;
        uint64_t SourceMip = 0;
    };

    std::vector<DownsamplingInvocationInputs> GenerateDownsamplingShaderInvocationInputs(
        Foundation::Name textureName,
        const HAL::TextureProperties& textureProperties,
        DownsamplingCBContent::Filter filter,
        DownsamplingStrategy strategy,
        std::optional<uint64_t> maxMipLevel = std::nullopt);

    void UpdateDownsamplingInputsWithTextureIndices(
        DownsamplingInvocationInputs& inputs, 
        const ResourceProvider* resourceProvider);

}
