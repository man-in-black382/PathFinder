#include "DenoiserMipGenerationRenderPass.hpp"
#include "DownsamplingCBContent.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserMipGenerationRenderPass::DenoiserMipGenerationRenderPass()
        : RenderPass("DenoiserMipGeneration") {}

    void DenoiserMipGenerationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
    }

    void DenoiserMipGenerationRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        auto frameIndex = scheduler->FrameNumber() % 2;

        scheduler->ReadTexture(ResourceNames::GBufferViewDepth[frameIndex]);
        scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingPreBlurred, { 0 });
        scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, { 0 });

        scheduler->WriteTexture(ResourceNames::GBufferViewDepth[frameIndex], { 1, 2, 3, 4 });
        scheduler->WriteTexture(ResourceNames::StochasticShadowedShadingPreBlurred, { 1, 2, 3, 4 });
        scheduler->WriteTexture(ResourceNames::StochasticUnshadowedShadingPreBlurred, { 1, 2, 3, 4 });
    }
     
    void DenoiserMipGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Downsampling);

        auto resourceProvider = context->GetResourceProvider();
        auto frameIndex = context->FrameNumber() % 2;

        // Downsample view depth
        DownsamplingCBContent cbContent{};
        cbContent.FilterType = DownsamplingCBContent::Filter::Min;
        cbContent.SourceTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferViewDepth[frameIndex]);
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth[frameIndex], 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth[frameIndex], 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth[frameIndex], 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth[frameIndex], 4);

        auto firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::GBufferViewDepth[frameIndex]).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });

        // Downsample shadowed luminance
        cbContent.FilterType = DownsamplingCBContent::Filter::Average;
        cbContent.SourceTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred);
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingPreBlurred, 4);

        firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::StochasticShadowedShadingPreBlurred).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });

        // Downsample unshadowed luminance
        cbContent.FilterType = DownsamplingCBContent::Filter::Average;
        cbContent.SourceTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred);
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingPreBlurred, 4);

        firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::StochasticUnshadowedShadingPreBlurred).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });
    }

}
