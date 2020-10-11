#pragma once

#include "PipelineResourceSchedulingInfo.hpp"

#include <Foundation/Name.hpp>
#include <Memory/GPUResourceProducer.hpp>


namespace PathFinder
{

    struct PipelineResourceStorageResource
    {
    public:
        struct DiffEntry
        {
        public:
            bool operator==(const DiffEntry& that) const;

            // Compare by name to detect new or deleted resources
            Foundation::Name ResourceName;

            // Compare by aliasing capability 
            bool CanBeAliased = true;

            // Compare by resource states because new state combination will require reallocation
            HAL::ResourceState ExpectedStates = HAL::ResourceState::Common;

            // Compare by total occupied memory
            uint64_t MemoryFootprint = 0;

            // Compare by lifetimes when aliasing is possible 
            uint64_t LifetimeStart = 0;
            uint64_t LifetimeEnd = 0;
        };

        PipelineResourceStorageResource(Foundation::Name resourceName, const HAL::ResourceFormat& format);

        PipelineResourceSchedulingInfo SchedulingInfo;
        Memory::GPUResourceProducer::TexturePtr Texture;
        Memory::GPUResourceProducer::BufferPtr Buffer;

        const Memory::GPUResource* GetGPUResource() const;
        Memory::GPUResource* GetGPUResource();

        DiffEntry GetDiffEntry() const;

    private:
        Foundation::Name mResourceName;

    public:
        inline Foundation::Name ResourceName() const { return mResourceName; }
    };

}