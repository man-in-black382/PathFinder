#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

#include "ShaderManager.hpp"
#include "RenderSurfaceDescription.hpp"
#include "PipelineStateProxy.hpp"
#include "ShaderFileNames.hpp"

#include <unordered_map>

namespace PathFinder
{
    using PSOName = Foundation::Name;
    using RootSignatureName = Foundation::Name;

    class PipelineStateManager
    {
    public:
        using GraphicsStateConfigurator = std::function<void(GraphicsStateProxy&)>;
        using ComputeStateConfigurator = std::function<void(ComputeStateProxy&)>;
        using RayTracingStateConfigurator = std::function<void(RayTracingStateProxy&)>;

        PipelineStateManager(HAL::Device* device, ShaderManager* shaderManager, const RenderSurfaceDescription& defaultRenderSurface);

        void StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature);
        void CreateGraphicsState(PSOName name, const GraphicsStateConfigurator& configurator);
        void CreateComputeState(PSOName name, const ComputeStateConfigurator& configurator);
        void CreateRayTracingState(PSOName name, const RayTracingStateConfigurator& configurator);

        const HAL::RootSignature* GetRootSignature(RootSignatureName name) const;
        const HAL::GraphicsPipelineState* GetGraphicsPipelineState(PSOName name) const;
        const HAL::ComputePipelineState* GetComputePipelineState(PSOName name) const;
        const HAL::RayTracingPipelineState* GetRayTracingPipelineState(PSOName name) const;

        const HAL::RootSignature* GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const;
        const HAL::RootSignature* GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const;

        const HAL::RootSignature& BaseRootSignature() const;
        const HAL::GraphicsPipelineState& DefaultGraphicsState() const;

        void CompileStates();

    private:
        void ConfigureDefaultStates();
        void BuildBaseRootSignature(); 

        ShaderManager* mShaderManager;
        RenderSurfaceDescription mDefaultRenderSurfaceDesc;
        
        HAL::Device* mDevice;
        HAL::RootSignature mBaseRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;
        std::unordered_map<PSOName, HAL::ComputePipelineState> mComputePSOs;
        std::unordered_map<PSOName, HAL::RayTracingPipelineState> mRayTracingPSOs;
        std::unordered_map<RootSignatureName, HAL::RootSignature> mRootSignatures;
    };

}
