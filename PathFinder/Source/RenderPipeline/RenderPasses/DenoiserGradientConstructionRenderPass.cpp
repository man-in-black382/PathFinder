#include "DenoiserGradientConstructionRenderPass.hpp"

#include "../Foundation/Gaussian.hpp"

namespace PathFinder
{

    DenoiserGradientConstructionRenderPass::DenoiserGradientConstructionRenderPass()
        : RenderPass("DenoiserGradientConstruction") {}

    void DenoiserGradientConstructionRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
       /* stateCreator->CreateComputeState(PSONames::DenoiserGradientConstruction, [](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DenoiserGradientConstruction.hlsl";
        });*/
    }

    void DenoiserGradientConstructionRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        //ResourceScheduler::NewTextureProperties gradientProperties{};
        //gradientProperties.ShaderVisibleFormat = HAL::ColorFormat::R16_Float;

        //scheduler->NewTexture(ResourceNames::StochasticShadowedShadingGradient, gradientProperties);
        //scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingGradient, gradientProperties);

        //scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingReprojected);
        //scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingReprojected);
        //scheduler->ReadTexture(ResourceNames::StochasticShadowedShadingOutput);
        //scheduler->ReadTexture(ResourceNames::StochasticUnshadowedShadingOutput);
    }
     
    void DenoiserGradientConstructionRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        /*     context->GetCommandRecorder()->ApplyPipelineState(PSONames::DenoiserGradientConstruction);

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
