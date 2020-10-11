#include "PipelineStateProxy.hpp"



namespace PathFinder
{

    HAL::ShaderTableIndex RayTracingStateProxy::AddCallableShader(const std::string& fileName, const std::string& entryPoint, std::optional<Foundation::Name> localRootSignatureName)
    {
        // PSO Implementation is expected to put shaders exactly in order they were put into this state proxy's array
        mCallableShaders.emplace_back(CallableShader{ fileName, entryPoint, localRootSignatureName });
        return mCallableShaders.size() - 1;
    }

    void RayTracingStateProxy::AddMissShader(const std::string& missShaderFileName, std::optional<Foundation::Name> localRootSignatureName)
    {
        mMissShaders.emplace_back(MissShader{ missShaderFileName, localRootSignatureName });
    }

    void RayTracingStateProxy::AddHitGroupShaders(const HitGroupShaderFileNames& fileNames, std::optional<Foundation::Name> localRootSignatureName)
    {
        mHitGroups.emplace_back(HitGroup{ fileNames, localRootSignatureName });
    }

}
