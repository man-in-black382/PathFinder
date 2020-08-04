#pragma once

#include <cstdint>
#include <vector>
#include <optional>

#include <glm/vec4.hpp>

#include "../Geometry/Dimensions.hpp"
#include "../ResourceProvider.hpp"

namespace PathFinder
{

    struct DownsamplingCBContent
    {
        enum class Filter : uint32_t
        { 
            Average = 0, Min = 1, Max = 2 
        };

        Filter FilterType = Filter::Average;
        uint32_t SourceTexIdx = 0;
        bool IsSourceTexSRV = true;
        uint32_t NumberOfOutputsToCompute = 0;
        glm::uvec4 OutputTexIndices;
        glm::uvec4 OutputsToWrite;
        glm::uvec2 InputSize;
        bool IsInputSizeOddVertically = false;
        bool IsInputSizeOddHorizontally = false;
    };

    enum class DownsamplingStrategy
    {
        WriteAllLevels, WriteOnlyLastLevel
    };

    struct DownsamplingInvocationInputs
    {
        DownsamplingCBContent CBContent;
        Geometry::Dimensions DispatchDimensions;
    };

    std::vector<DownsamplingInvocationInputs> GenerateDownsamplingShaderInvocationInputs(
        const ResourceProvider& resourceProvider,
        Foundation::Name textureName,
        DownsamplingCBContent::Filter filter,
        DownsamplingStrategy strategy,
        std::optional<uint64_t> maxMipLevel = std::nullopt);

}
