#include "DeferredLightingRenderPass.hpp"

namespace PathFinder
{

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {}

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature deferedLightingSinature = stateCreator->CloneBaseRootSignature();
        deferedLightingSinature.AddConstantsParameter(HAL::RootConstantsParameter{ 1, 0, 0 }); // 1 uint light index
        deferedLightingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Lights buffer
        stateCreator->StoreRootSignature(RootSignatureNames::DeferredLighting, std::move(deferedLightingSinature));

        stateCreator->CreateComputeState(PSONames::DeferredLighting, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DeferredLightingRenderPass.hlsl";
            state.RootSignatureName = RootSignatureNames::DeferredLighting;
        });
    }
      
    void DeferredLightingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::DeferredLightingOutput);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
    }  

    void DeferredLightingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);

        Memory::Texture* ltcLut0 = context->GetContent()->GetScene()->LTC_LUT0();
        Memory::Texture* ltcLut1 = context->GetContent()->GetScene()->LTC_LUT1();

        auto lut0Size = ltcLut0->HALTexture()->Dimensions();
        auto lut1Size = ltcLut1->HALTexture()->Dimensions();

        assert_format(lut0Size.Width == lut0Size.Height && 
            lut0Size.Width == lut1Size.Height &&
            lut0Size.Width == lut1Size.Width,
            "LUTs should be square and equal size");

        DeferredLightingCBContent cbContent{}; 
        cbContent.GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent.GBufferDepthTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferDepthStencil);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);
        cbContent.LTC_LUT0_Index = ltcLut0->GetOrCreateSRDescriptor()->IndexInHeapRange();
        cbContent.LTC_LUT1_Index = ltcLut1->GetOrCreateSRDescriptor()->IndexInHeapRange();
        cbContent.LTC_LUT_Size = lut0Size.Width;

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        Scene* scene = context->GetContent()->GetScene();
        SceneGPUStorage* sceneGPUStorage = context->GetContent()->GetSceneGPUStorage();

        context->GetCommandRecorder()->BindExternalBuffer(*sceneGPUStorage->LightInstanceTable(), 0, 0, HAL::ShaderRegister::ShaderResource);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);

        for (const DiskLight& light : scene->DiskLights())
        {
            context->GetCommandRecorder()->SetRootConstants(light.GPULightTableIndex(), 0, 0);
            context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
        }
        
    }

}
