#include "DisplacementDistanceMapRenderPass.hpp"

namespace PathFinder
{

    DisplacementDistanceMapRenderPass::DisplacementDistanceMapRenderPass()
        : RenderPass("DisplaycmentDistanceMapGeneration", Purpose::Setup) {}

    void DisplacementDistanceMapRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature signature = stateCreator->CloneBaseRootSignature();
        signature.AddDescriptorParameter(HAL::RootUnorderedAccessParameter{ 0, 0 }); // UAV Counter | u0 - s0

        stateCreator->StoreRootSignature(RootSignatureNames::DisplacementDistanceMapGeneration, std::move(signature));

        stateCreator->CreateComputeState(PSONames::DisplacementDistanceMapGeneration, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DisplacementDistanceMapGenerationRenderPass.hlsl";
            state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        });
    }
     
    void DisplacementDistanceMapRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->WillUseRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();
    } 

    void DisplacementDistanceMapRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DisplacementDistanceMapGeneration);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
