#pragma once

#include "PipelineResourceSchedulingInfo.hpp"

#include "../Foundation/Name.hpp"
#include "../Memory/GPUResourceProducer.hpp"


namespace PathFinder
{

    struct PipelineResourceStorageResource
    {
    public:
        struct DiffEntry
        {
        public:
            bool operator==(const DiffEntry& that) const;

            Foundation::Name ResourceName;
            uint64_t MemoryFootprint;
        };

        PipelineResourceStorageResource(Foundation::Name resourceName, const HAL::ResourceFormat& format, uint64_t resourceCount);

        // An array of resources can be requested per resource name
        PipelineResourceSchedulingInfo SchedulingInfo;
        std::vector<Memory::GPUResourceProducer::TexturePtr> Textures;
        std::vector<Memory::GPUResourceProducer::BufferPtr> Buffers;

        const Memory::GPUResource* GetGPUResource(uint64_t resourceIndex = 0) const;
        Memory::GPUResource* GetGPUResource(uint64_t resourceIndex = 0);

        const Memory::Texture* GetTexture(uint64_t resourceIndex = 0) const;
        Memory::Texture* GetTexture(uint64_t resourceIndex = 0);

        const Memory::Buffer* GetBuffer(uint64_t resourceIndex = 0) const;
        Memory::Buffer* GetBuffer(uint64_t resourceIndex = 0);

        uint64_t ResourceCount() const;
        DiffEntry GetDiffEntry() const;

    private:
        Foundation::Name mResourceName;

    public:
        inline Foundation::Name ResourceName() const { return mResourceName; }
    };

}