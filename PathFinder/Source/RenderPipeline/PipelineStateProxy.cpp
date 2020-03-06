#include "PipelineStateProxy.hpp"

#include "../Foundation/Assert.hpp"

namespace PathFinder
{

    void RayTracingStateProxy::AddMissShader(const std::string& missShaderFileName, std::optional<Foundation::Name> localRootSignatureName)
    {
        mMissShaders.emplace_back(MissShader{ missShaderFileName, localRootSignatureName });
    }

    void RayTracingStateProxy::AddHitGroupShaders(const HitGroupShaderFileNames& fileNames, std::optional<Foundation::Name> localRootSignatureName)
    {
        mHitGroups.emplace_back(HitGroup{ fileNames, localRootSignatureName });
    }

}
