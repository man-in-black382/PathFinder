#include "SMAABlendingWeightCalculationRenderPass.hpp"

namespace PathFinder
{

    SMAABlendingWeightCalculationRenderPass::SMAABlendingWeightCalculationRenderPass()
        : RenderPass("SMAABlendingWeightCalculation") {}

    void SMAABlendingWeightCalculationRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateGraphicsState(PSONames::SMAABlendingWeightCalculation, [](GraphicsStateProxy& state)
        {
            state.VertexShaderFileName = "SMAABlendingWeightCalculation.hlsl";
            state.PixelShaderFileName = "SMAABlendingWeightCalculation.hlsl";
            state.PrimitiveTopology = HAL::PrimitiveTopology::TriangleList;
            state.DepthStencilState.SetDepthTestEnabled(false);
            state.RenderTargetFormats = { HAL::ColorFormat::RGBA16_Float };
        });
    }

    void SMAABlendingWeightCalculationRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->ReadTexture(ResourceNames::SMAADetectedEdges);
        scheduler->NewRenderTarget(ResourceNames::SMAABlendingWeights, ResourceScheduler::NewTextureProperties{ HAL::ColorFormat::RGBA16_Float });
    }

    void SMAABlendingWeightCalculationRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::SMAABlendingWeightCalculation);

        const Scene* scene = context->GetContent()->GetScene(); 

        SMAABlendingWeightCalculationCBContent cbContent{};
        cbContent.EdgesTexIdx = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::SMAADetectedEdges);
        cbContent.AreaTexIdx = scene->SMAAAreaTexture()->GetSRDescriptor()->IndexInHeapRange();
        cbContent.SearchTexIdx = scene->SMAASearchTexture()->GetSRDescriptor()->IndexInHeapRange();

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRenderTarget(ResourceNames::SMAABlendingWeights);
        context->GetCommandRecorder()->Draw(DrawablePrimitive::Triangle());
    }

}
