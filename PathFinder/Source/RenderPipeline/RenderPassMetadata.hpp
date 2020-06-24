#pragma once

#include "../Foundation/Name.hpp"

namespace PathFinder
{
    enum class RenderPassPurpose
    {
        // Every-frame render pass.
        Default,
        // One-time render pass to setup common states and/or resources.
        Setup,
        // One-time render pass for asset preprocessing.
        // Asset resources will be specifically transitioned
        // to appropriate states before and after execution of such passes.
        AssetProcessing
    };

    enum class RenderPassExecutionQueue : uint64_t
    {
        Graphics = 0, AsyncCompute = 1
    };

    struct RenderPassMetadata
    {
        Foundation::Name Name;
        RenderPassPurpose Purpose = RenderPassPurpose::Default;
    };

}
