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

        scheduler->ReadWriteTexture({ ResourceNames::GBufferViewDepth, frameIndex });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        scheduler->ReadWriteTexture({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
    }
     
    void DenoiserMipGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Downsampling);

        auto resourceProvider = context->GetResourceProvider();
        auto frameIndex = context->FrameNumber() % 2;

        // Downsample view depth
        DownsamplingCBContent cbContent{};
        cbContent.FilterType = DownsamplingCBContent::Filter::Min;
        cbContent.SourceTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::GBufferViewDepth, frameIndex });
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::GBufferViewDepth, frameIndex }, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::GBufferViewDepth, frameIndex }, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::GBufferViewDepth, frameIndex }, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::GBufferViewDepth, frameIndex }, 4);

        auto firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::GBufferViewDepth).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });

        // Downsample shadowed luminance
        cbContent.FilterType = DownsamplingCBContent::Filter::Average;
        cbContent.SourceTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 4);

        firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::ShadingStochasticShadowedOutput).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });

        // Downsample unshadowed luminance
        cbContent.FilterType = DownsamplingCBContent::Filter::Average;
        cbContent.SourceTexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx });
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, ResourceIndices::StochasticShadingCurrentFrameOutputArrayIdx }, 4);

        firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::ShadingStochasticUnshadowedOutput).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });
    }

}
