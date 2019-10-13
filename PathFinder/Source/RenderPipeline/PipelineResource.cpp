#include "PipelineResource.hpp"

namespace PathFinder
{

    PipelineResource::PipelineResource() {}

    PipelineResource::PassMetadata& PipelineResource::AllocateMetadateForPass(Foundation::Name passName)
    {
        auto [iter, success] = mPerPassData.emplace(passName, PassMetadata{});
        return iter->second;
    }

    const PipelineResource::PassMetadata* PipelineResource::GetMetadataForPass(Foundation::Name passName) const
    {
        auto it = mPerPassData.find(passName);
        return it == mPerPassData.end() ? nullptr : &it->second;
    }

}
