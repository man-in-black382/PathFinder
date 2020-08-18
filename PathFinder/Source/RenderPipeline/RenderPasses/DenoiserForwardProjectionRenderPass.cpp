#include "DenoiserForwardProjectionRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserForwardProjectionRenderPass::DenoiserForwardProjectionRenderPass()
        : RenderPass("DenoiserForwardProjection") {}

    void DenoiserForwardProjectionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateComputeState(PSONames::DenoiserForwardProjection, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserForwardProjection.hlsl";
        });
    }

    void DenoiserForwardProjectionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto currentFrameIndex = scheduler->FrameNumber() % 2;
        auto previousFrameIndex = (scheduler->FrameNumber() - 1) % 2;

        Geometry::Dimensions gradientTexturesSize = scheduler->DefaultRenderSurfaceDesc().Dimensions().XYMultiplied(1.0 / 3.0);

        ResourceScheduler::NewTextureProperties gradientSamplePositionsProps{ HAL::ColorFormat::R8_Unsigned, HAL::TextureKind::Texture2D, gradientTexturesSize };
        ResourceScheduler::NewTextureProperties gradientProps{ HAL::ColorFormat::RG16_Float, HAL::TextureKind::Texture2D, gradientTexturesSize };

        gradientSamplePositionsProps.Flags = ResourceScheduler::Flags::CrossFrameRead;

       /* scheduler->NewTexture(ResourceNames::DenoiserGradientSamplePositions[currentFrameIndex], gradientSamplePositionsProps);
        scheduler->NewTexture(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex], ResourceScheduler::MipSet::Empty(), gradientSamplePositionsProps);
        scheduler->NewTexture(ResourceNames::StochasticShadingGradient, gradientProps);*/

        //scheduler->ReadTexture(ResourceNames::GBufferViewDepth[previousFrameIndex]);
       /* scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingDenoised[previousFrameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingDenoised[previousFrameIndex]);*/
        scheduler->ReadTexture(ResourceNames::RngSeeds[previousFrameIndex]);
        //scheduler->ReadTexture(ResourceNames::DenoiserGradientSamplePositions[previousFrameIndex]);

        scheduler->AliasAndWriteTexture(ResourceNames::RngSeeds[currentFrameIndex], ResourceNames::RngSeedsCorrelated);
    }
     
    void DenoiserForwardProjectionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        /*     context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserForwardProjection);

             auto resourceProvider = context->GetResourceProvider();

             DenoiserGradientConstructionCBContent cbContent{};
             cbContent.ShadingShadowedTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingOutput);
             cbContent.ShadingUnshadowedTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingOutput);
             cbContent.ShadingShadowedHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingReprojected);
             cbContent.ShadingUnshadowedHistoryTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingReprojected);
             cbContent.ShadingShadowedGradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingGradient);
             cbContent.ShadingUnshadowedGradientTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingGradient);

             context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
             context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 16, 16 });*/
    }

}
