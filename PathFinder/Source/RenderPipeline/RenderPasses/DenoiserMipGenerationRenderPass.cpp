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
        scheduler->ReadWriteTexture(ResourceNames::GBufferViewDepth);
        scheduler->ReadWriteTexture(ResourceNames::ShadingStochasticShadowedOutput);
        scheduler->ReadWriteTexture(ResourceNames::ShadingStochasticUnshadowedOutput);
    }
     
    void DenoiserMipGenerationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::AveragindDownsampling);

        auto resourceProvider = context->GetResourceProvider();

        // Downsample view depth
        DownsamplingCBContent cbContent{};
        cbContent.SourceTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth, 0);
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::GBufferViewDepth, 4);

        auto firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::GBufferViewDepth).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });

        // Downsample shadowed luminance
        cbContent.SourceTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput, 0);
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput, 4);

        firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::ShadingStochasticShadowedOutput).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });

        // Downsample unshadowed luminance
        cbContent.SourceTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput, 0);
        cbContent.Destination0TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput, 1);
        cbContent.Destination1TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput, 2);
        cbContent.Destination2TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput, 3);
        cbContent.Destination3TexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput, 4);

        firstMipDimensions = resourceProvider->GetTextureProperties(ResourceNames::ShadingStochasticUnshadowedOutput).Dimensions.XYMultiplied(0.5);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->Dispatch(firstMipDimensions, { 8, 8 });
    }

}
