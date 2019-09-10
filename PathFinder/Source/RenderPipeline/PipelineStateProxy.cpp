#include "PipelineStateProxy.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    void RayTracingStateProxy::AddShaders(
        const RayTracingShaderFileNames& shaderFileNames, 
        const HAL::RayTracingShaderConfig& config,
        std::optional<Foundation::Name> localRootSignatureName)
    {
        mShaderInfos.emplace_back(shaderFileNames, config, localRootSignatureName);
    }

}
