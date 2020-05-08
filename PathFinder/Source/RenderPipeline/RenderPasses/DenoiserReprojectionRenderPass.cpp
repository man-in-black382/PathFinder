#include "DenoiserReprojectionRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserReprojectionRenderPass::DenoiserReprojectionRenderPass()
        : RenderPass("DenoiserReprojection") {}

    void DenoiserReprojectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserReprojection, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserReprojection.hlsl";
        });
    }

    void DenoiserReprojectionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        ResourceScheduler::NewTextureProperties frameCountProperties{};
        frameCountProperties.ShaderVisibleFormat = HAL::ColorFormat::R8_Usigned_Norm;
        frameCountProperties.TextureCount = 2;

        scheduler->NewTexture(ResourceNames::DenoiserReprojectedFramesCount, frameCountProperties);

        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture({ ResourceNames::GBufferDepthStencil, 0 });
        scheduler->ReadTexture({ ResourceNames::GBufferDepthStencil, 1 });
        scheduler->ReadTexture({ ResourceNames::DenoiserReprojectedFramesCount, (scheduler->FrameNumber() - 1) % 2 });
    }
     
    void DenoiserReprojectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserReprojection);

        auto resourceProvider = context->GetResourceProvider();

        auto previousFrameIndex = (context->FrameNumber() - 1) % 2;
        auto frameIndex = context->FrameNumber() % 2;

        DenoiserReprojectionCBContent cbContent{};
        cbContent.GBufferTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferRT0);
        cbContent.PreviousDepthTextureIndex = resourceProvider->GetSRTextureIndex({ ResourceNames::GBufferDepthStencil, previousFrameIndex });
        cbContent.CurrentDepthTextureIndex = resourceProvider->GetSRTextureIndex({ ResourceNames::GBufferDepthStencil, frameIndex });
        cbContent.PreviousAccumulationCounterTextureIndex = resourceProvider->GetSRTextureIndex({ ResourceNames::DenoiserReprojectedFramesCount, previousFrameIndex });
        cbContent.CurrentAccumulationCounterTextureIndex = resourceProvider->GetUATextureIndex({ ResourceNames::DenoiserReprojectedFramesCount, frameIndex });

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });
    }

}
