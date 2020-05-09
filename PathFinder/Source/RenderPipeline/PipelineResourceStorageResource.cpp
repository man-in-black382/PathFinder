#include "PipelineResourceStorageResource.hpp"

#include "../Foundation/StringUtils.hpp"
#include "../Foundation/Assert.hpp"
#include "../Foundation/STDHelpers.hpp"

namespace PathFinder
{

    PipelineResourceStorageResource::PipelineResourceStorageResource(Foundation::Name resourceName, const HAL::ResourceFormat& format, uint64_t resourceCount)
        : mResourceName{ resourceName }, SchedulingInfo{ resourceName, format, resourceCount } {}

    const Memory::GPUResource* PipelineResourceStorageResource::GetGPUResource(uint64_t resourceIndex) const
    {
        if (resourceIndex + 1 <= Textures.size()) return Textures[resourceIndex].get();
        else if (resourceIndex + 1 <= Buffers.size()) return Buffers[resourceIndex].get();
        else return nullptr;
    }

    Memory::GPUResource* PipelineResourceStorageResource::GetGPUResource(uint64_t resourceIndex)
    {
        if (resourceIndex + 1 <= Textures.size()) return Textures[resourceIndex].get();
        else if (resourceIndex + 1 <= Buffers.size()) return Buffers[resourceIndex].get();
        else return nullptr;
    }

    const Memory::Texture* PipelineResourceStorageResource::GetTexture(uint64_t resourceIndex) const
    {
        return resourceIndex + 1 <= Textures.size() ? Textures[resourceIndex].get() : nullptr;
    }

    Memory::Texture* PipelineResourceStorageResource::GetTexture(uint64_t resourceIndex)
    {
        return resourceIndex + 1 <= Textures.size() ? Textures[resourceIndex].get() : nullptr;
    }

    const Memory::Buffer* PipelineResourceStorageResource::GetBuffer(uint64_t resourceIndex) const
    {
        return resourceIndex + 1 <= Buffers.size() ? Buffers[resourceIndex].get() : nullptr;
    }

    Memory::Buffer* PipelineResourceStorageResource::GetBuffer(uint64_t resourceIndex)
    {
        return resourceIndex + 1 <= Buffers.size() ? Buffers[resourceIndex].get() : nullptr;
    }

    uint64_t PipelineResourceStorageResource::ResourceCount() const
    {
        return Textures.empty() ? Buffers.size() : Textures.size();
    }

    PipelineResourceStorageResource::DiffEntry PipelineResourceStorageResource::GetDiffEntry() const
    {
        return { mResourceName, SchedulingInfo.TotalRequiredMemory() };
    }

    bool PipelineResourceStorageResource::DiffEntry::operator==(const DiffEntry& that) const
    {
        // Pipeline Resource is identified by its name and memory footprint which is sufficient to understand when
        // resource allocation, reallocation or deallocation is required.
        return this->ResourceName == that.ResourceName && this->MemoryFootprint == that.MemoryFootprint;
    }

}
