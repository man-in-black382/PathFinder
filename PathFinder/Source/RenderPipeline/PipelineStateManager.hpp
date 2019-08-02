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

        HAL::PipelineState& GetPipelineState(PSOName name);

        void CompileStates();

    private:
        void ConfigureDefaultStates();
        void BuildUniversalRootSignature();

        RenderSurface mDefaultRenderSurface;
        
        HAL::Device* mDevice;
        HAL::RootSignature mUniversalRootSignature;
        HAL::GraphicsPipelineState mDefaultGraphicsState;

        std::unordered_map<PSOName, HAL::GraphicsPipelineState> mGraphicPSOs;

    public:
        inline const HAL::RootSignature& UniversalRootSignature() const { return mUniversalRootSignature; }
    };

}
