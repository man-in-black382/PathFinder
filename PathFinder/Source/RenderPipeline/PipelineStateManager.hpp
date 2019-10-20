#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

#include "ShaderManager.hpp"
#include "RenderSurfaceDescription.hpp"

#include <unordered_map>

namespace PathFinder
{
    using PSOName = Foundation::Name;
    using RootSignatureName = Foundation::Name;

    class PipelineStateManager
    {
    public:
        PipelineStateManager(HAL::Device* device, ShaderManager* shaderManager, const RenderSurfaceDescription& defaultRenderSurface);

        void StoreRootSignature(RootSignatureName name, HAL::RootSignature&& signature);
        void StorePipelineState(PSOName name, HAL::GraphicsPipelineState&& state);
        void StorePipelineState(PSOName name, HAL::ComputePipelineState&& state);
        void StorePipelineState(PSOName name, HAL::RayTracingPipelineState&& state);

        const HAL::RootSignature* GetRootSignature(RootSignatureName name) const;
        const HAL::GraphicsPipelineState* GetGraphicsPipelineState(PSOName name) const;
        const HAL::ComputePipelineState* GetComputePipelineState(PSOName name) const;
        const HAL::RayTracingPipelineState* GetRayTracingPipelineState(PSOName name) const;

        const HAL::RootSignature* GetNamedRootSignatureOrDefault(std::optional<RootSignatureName> name) const;
        const HAL::RootSignature* GetNamedRootSignatureOrNull(std::optional<RootSignatureName> name) const;

        const HAL::RootSignature& BaseRootSignature() const;
        const HAL::GraphicsPipelineState& DefaultGraphicsState() const;
        
        HAL::ComputePipelineState CreateComputeState() const;
        HAL::RayTracingPipelineState CreateRayTracingState() const;

        void CompileStates();

    private:
        void ConfigureDefaultStates();
        void BuildBaseRootSignature(); 

        ShaderManager* mShaderManager;
        RenderSurfaceDescription mDefaultRenderSurface;
        
        HAL::Device* mDevice;
        HAL::RootSignature mBaseRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;
        std::unordered_map<PSOName, HAL::ComputePipelineState> mComputePSOs;
        std::unordered_map<PSOName, HAL::RayTracingPipelineState> mRayTracingPSOs;
        std::unordered_map<RootSignatureName, HAL::RootSignature> mRootSignatures;

    public:
        ShaderManager* GetShaderManager() const { return mShaderManager; }
    };

}
