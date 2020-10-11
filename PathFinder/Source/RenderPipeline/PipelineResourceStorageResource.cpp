#include "PipelineResourceStorageResource.hpp"

#include <Foundation/StringUtils.hpp>

#include <Foundation/STDHelpers.hpp>

namespace PathFinder
{

    PipelineResourceStorageResource::PipelineResourceStorageResource(Foundation::Name resourceName, const HAL::ResourceFormat& format)
        : mResourceName{ resourceName }, SchedulingInfo{ resourceName, format } {}

    const Memory::GPUResource* PipelineResourceStorageResource::GetGPUResource() const
    {
        if (Texture) return Texture.get();
        else if (Buffer) return Buffer.get();
        else return nullptr;
    }

    Memory::GPUResource* PipelineResourceStorageResource::GetGPUResource()
    {
        if (Texture) return Texture.get();
        else if (Buffer) return Buffer.get();
        else return nullptr;
    }

    PipelineResourceStorageResource::DiffEntry PipelineResourceStorageResource::GetDiffEntry() const
    {
        return { 
            mResourceName, 
            SchedulingInfo.CanBeAliased, 
            SchedulingInfo.ExpectedStates(), 
            SchedulingInfo.TotalRequiredMemory(), 
            SchedulingInfo.AliasingLifetime.first, 
            SchedulingInfo.AliasingLifetime.second 
        };
    }

    bool PipelineResourceStorageResource::DiffEntry::operator==(const DiffEntry& that) const
    {
        // Pipeline Resource is identified by its name, memory footprint and lifetime,
        // which is sufficient to understand when
        // resource allocation, reallocation or deallocation is required.
        bool equal = 
            ResourceName == that.ResourceName &&
            MemoryFootprint == that.MemoryFootprint &&
            CanBeAliased == that.CanBeAliased &&
            ExpectedStates == that.ExpectedStates;

        // Compare timelines only if resource can be aliased
        if (CanBeAliased)
        {
            equal = equal && LifetimeStart == that.LifetimeStart && LifetimeEnd == that.LifetimeEnd;
        }

        return equal;
    }

}
