#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/ResourceState.hpp"
#include "../HardwareAbstractionLayer/Resource.hpp"
#include "../HardwareAbstractionLayer/ResourceBarrier.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    class PipelineResource
    {
    public:
        struct PassMetadata
        {
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            bool IsRTDescriptorRequested = false;
            bool IsDSDescriptorRequested = false;
            bool IsSRDescriptorRequested = false;
            bool IsUADescriptorRequested = false;
            bool IsCBDescriptorRequested = false;
        };

        PipelineResource();

        PassMetadata& AllocateMetadateForPass(Foundation::Name passName);
        const PassMetadata* GetMetadataForPass(Foundation::Name passName) const;

        std::unique_ptr<HAL::Resource> Resource;

    private:
        std::unordered_map<Foundation::Name, PassMetadata> mPerPassData;
    };

}
