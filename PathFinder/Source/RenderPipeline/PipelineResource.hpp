#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/Texture.hpp"
#include "../HardwareAbstractionLayer/Buffer.hpp"

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
            std::optional<HAL::ColorFormat> ShaderVisibleFormat;
            HAL::ResourceState RequiredState = HAL::ResourceState::Common;
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

    class TexturePipelineResource : public PipelineResource<HAL::Texture> {};
    class BufferPipelineResource : public PipelineResource<HAL::Buffer> {};

}

#include "PipelineResource.inl"