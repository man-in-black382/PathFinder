#pragma once

#include "../PipelineResourceStorage.hpp"
#include "../RenderPassGraph.hpp"

#include <Foundation/Name.hpp>
#include <HardwareAbstractionLayer/ShaderRegister.hpp>

namespace PathFinder
{

    class ResourceProvider
    {
    public:
        ResourceProvider(const PipelineResourceStorage* storage, const RenderPassGraph* passGraph, uint64_t graphNodeIndex);
       
        uint32_t GetUABufferIndex(Foundation::Name bufferName) const;
        uint32_t GetUATextureIndex(Foundation::Name textureName, uint8_t mipLevel = 0) const;
        uint32_t GetSRTextureIndex(Foundation::Name textureName, uint8_t mipLevel = 0) const;
        uint32_t GetSamplerIndex(Foundation::Name samplerName) const;
        const HAL::TextureProperties& GetTextureProperties(Foundation::Name resourceName) const;
        const HAL::BufferProperties& GetBufferProperties(Foundation::Name resourceName) const;

    private:
        const PipelineResourceStorage* mResourceStorage;
        const RenderPassGraph* mPassGraph;
        uint64_t mGraphNodeIndex;
    };

}
