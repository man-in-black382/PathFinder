#pragma once

#include <HardwareAbstractionLayer/PipelineState.hpp>
#include <Foundation/Name.hpp>

#include <vector>
#include <string>

namespace PathFinder
{

    class GraphicsStateProxy
    {
    public:
        std::string VertexShaderFileName;
        std::string PixelShaderFileName;
        std::optional<std::string> GeometryShaderFileName;
        HAL::BlendState BlendState;
        HAL::RasterizerState RasterizerState;
        HAL::DepthStencilState DepthStencilState;
        HAL::DepthStencilFormat DepthStencilFormat;
        HAL::PrimitiveTopology PrimitiveTopology;
        std::vector<HAL::GraphicsPipelineState::RenderTargetFormat> RenderTargetFormats;
        std::optional<Foundation::Name> RootSignatureName;
    };

    class ComputeStateProxy
    {
    public:
        std::string ComputeShaderFileName;
        std::optional<Foundation::Name> RootSignatureName;
    };

    class RayTracingStateProxy
    {
    public:
        struct HitGroupShaderFileNames
        {
            std::optional<std::string> ClosestHitShaderFileName;
            std::optional<std::string> AnyHitShaderFileName;
            std::optional<std::string> IntersectionShaderFileName;
        };

        struct HitGroup
        {
            HitGroupShaderFileNames ShaderFileNames;
            std::optional<Foundation::Name> LocalRootSignatureName;
        };

        struct MissShader
        {
            std::string MissShaderFileName;
            std::optional<Foundation::Name> LocalRootSignatureName;
        };

        struct CallableShader
        {
            std::string ShaderFileName;
            std::string EntryPointName;
            std::optional<Foundation::Name> LocalRootSignatureName;
        };

        HAL::ShaderTableIndex AddCallableShader(const std::string& fileName, const std::string& entryPoint, std::optional<Foundation::Name> localRootSignatureName = std::nullopt);
        void AddMissShader(const std::string& missShaderFileName, std::optional<Foundation::Name> localRootSignatureName = std::nullopt);
        void AddHitGroupShaders(const HitGroupShaderFileNames& fileNames, std::optional<Foundation::Name> localRootSignatureName = std::nullopt);

        HAL::RayTracingPipelineConfig PipelineConfig{ 0 };
        HAL::RayTracingShaderConfig ShaderConfig{ 0, 0 };
        std::string RayGenerationShaderFileName;
        std::optional<Foundation::Name> RayGenerationLocalRootSignatureName;
        std::optional<Foundation::Name> GlobalRootSignatureName;

    private:
        std::vector<HitGroup> mHitGroups;
        std::vector<MissShader> mMissShaders;
        std::vector<CallableShader> mCallableShaders;

    public:
        const auto& HitGroups() const { return mHitGroups; }
        const auto& MissShaders() const { return mMissShaders; }
        const auto& CallableShaders() const { return mCallableShaders; }
    };

}
