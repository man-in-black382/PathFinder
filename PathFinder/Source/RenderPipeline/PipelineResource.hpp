#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/BufferResource.hpp"

#include <functional>
#include <optional>

namespace PathFinder
{

    template <class ResourceT>
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

        PassMetadata& AllocateMetadateForPass(Foundation::Name passName);
        const PassMetadata* GetMetadataForPass(Foundation::Name passName) const;

        std::unique_ptr<ResourceT> Resource;

    private:
        std::unordered_map<Foundation::Name, PassMetadata> mPerPassData;
    };

    class TexturePipelineResource : public PipelineResource<HAL::TextureResource> {};
    class BufferPipelineResource : public PipelineResource<HAL::BufferResource<uint8_t>> {};

}

#include "PipelineResource.inl"