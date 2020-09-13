#include "SMAANeighborhoodBlendingRenderPass.hpp"

namespace PathFinder
{

    SMAANeighborhoodBlendingRenderPass::SMAANeighborhoodBlendingRenderPass()
        : RenderPass("SMAANeighborhoodBlending") {}

    void SMAANeighborhoodBlendingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::SMAANeighborhoodBlending, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "SMAANeighborhoodBlending.hlsl";
            state.PixelShaderFileName = "SMAANeighborhoodBlending.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.DepthStencilState.SetDepthTestEnabled(false);
        });
    }

    void SMAANeighborhoodBlendingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::ToneMappingOutput);
        scheduler->ReadTexture(ResourceNames::SMAABlendingWeights);
        scheduler->NewRenderTarget(ResourceNames::SMAAAntialiased);
    }

    void SMAANeighborhoodBlendingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SMAANeighborhoodBlending);

        SMAANeighborhoodBlendingCBContent cbContent{};
        cbContent.InputImageTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::ToneMappingOutput);
        cbContent.BlendingWeightsTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::SMAABlendingWeights);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::SMAAAntialiased);
        context->GetCommandRecorder()->Draw(DrawablePrimitive::Triangle());
    }

}
