#pragma once

#include "../HardwareAbstractionLayer/PipelineState.hpp"
#include "../Foundation/Name.hpp"
#include "ShaderFileNames.hpp"
#include <vector>

namespace PathFinder
{

    class GraphicsStateProxy
    {
    public:
        GraphicsShaderFileNames ShaderFileNames;
        HAL::BlendState BlendState;
        HAL::RasterizerState RasterizerState;
        HAL::DepthStencilState DepthStencilState;
        HAL::InputAssemblerLayout InputLayout;
        HAL::ResourceFormat::DepthStencil DepthStencilFormat;
        HAL::PrimitiveTopology PrimitiveTopology;
        std::vector<HAL::GraphicsPipelineState::RenderTargetFormat> RenderTargetFormats;
        std::optional<Foundation::Name> RootSignatureName;
    };

    class ComputeStateProxy
    {
    public:
        ComputeShaderFileNames ShaderFileNames;
        std::optional<Foundation::Name> RootSignatureName;
    };

    class RayTracingStateProxy
    {
    public:
        struct ShaderInfo
        {
            RayTracingShaderFileNames ShaderFileNames;
            HAL::RayTracingShaderConfig Config;
            std::optional<Foundation::Name> LocalRootSignatureName;

            ShaderInfo(RayTracingShaderFileNames fileNames, HAL::RayTracingShaderConfig config, std::optional<Foundation::Name> localRootSig)
                : ShaderFileNames{ fileNames }, Config{ config }, LocalRootSignatureName{ localRootSig } {}
        };

        void AddShaders(
            const RayTracingShaderFileNames& shaderFileNames, 
            const HAL::RayTracingShaderConfig& config,
            std::optional<Foundation::Name> localRootSignatureName = std::nullopt
        );

        HAL::RayTracingPipelineConfig PipelineConfig;
        std::optional<Foundation::Name> GlobalRootSignatureName;

    private:
        std::vector<ShaderInfo> mShaderInfos;

    public:
        const auto& ShaderInfos() const { return mShaderInfos; }
    };

}
