#pragma once

#include "../Foundation/Name.hpp"
#include "../HardwareAbstractionLayer/PipelineState.hpp"

#include "RenderSurface.hpp"
#include "IPipelineStateManager.hpp"

#include <unordered_map>

namespace PathFinder
{

    class PipelineStateManager : public IPipelineStateManager
    {
    public:
        PipelineStateManager(HAL::Device* device, const RenderSurface& defaultRenderSurface);

        virtual HAL::GraphicsPipelineState CloneDefaultGraphicsState() override;
        virtual HAL::GraphicsPipelineState CloneExistingGraphicsState(PSOName name) override;
        virtual HAL::ComputePipelineState CloneDefaultComputeState() override;
        virtual HAL::ComputePipelineState CloneExistingComputeState(PSOName name) override;
        virtual HAL::RootSignature CloneBaseRootSignature() override;

        virtual void StoreRootSignature(RootSignatureName name, const HAL::RootSignature& signature) override;
        virtual void StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso, RootSignatureName assosiatedSignatureName) override;
        virtual void StoreComputeState(PSOName name, const HAL::ComputePipelineState& pso, RootSignatureName assosiatedSignatureName) override;

        const HAL::RootSignature& GetRootSignature(RootSignatureName name) const;
        const HAL::PipelineState& GetPipelineState(PSOName name) const;

        void CompileStates();

    private:
        void ConfigureDefaultStates();
        void BuildBaseRootSignature(); 

        RenderSurface mDefaultRenderSurface;
        
        HAL::Device* mDevice;
        HAL::RootSignature mBaseRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;
        HAL::ComputePipelineState mDefaultComputeState;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;
        std::unordered_map<PSOName, HAL::ComputePipelineState> mComputePSOs;
        std::unordered_map<RootSignatureName, HAL::RootSignature> mRootSignatures;
    };

}
