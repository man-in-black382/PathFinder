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
        virtual void StoreGraphicsState(PSOName name, const HAL::GraphicsPipelineState& pso) override;

        virtual HAL::ComputePipelineState CloneDefaultComputeState() override;
        virtual HAL::ComputePipelineState CloneExistingComputeState(PSOName name) override;
        virtual void StoreComputeState(PSOName name, const HAL::ComputePipelineState& pso) override;

        const HAL::PipelineState& GetPipelineState(PSOName name) const;

        void CompileStates();

    private:
        void ConfigureDefaultStates();
        void BuildUniversalRootSignature();

        RenderSurface mDefaultRenderSurface;
        
        HAL::Device* mDevice;
        HAL::RootSignature mUniversalRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;
        HAL::ComputePipelineState mDefaultComputeState;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;
        std::unordered_map<PSOName, HAL::ComputePipelineState> mComputePSOs;

    public:
        inline const HAL::RootSignature& UniversalRootSignature() const { return mUniversalRootSignature; }
    };

}
