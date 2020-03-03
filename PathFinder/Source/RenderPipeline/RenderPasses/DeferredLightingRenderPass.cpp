#include "DeferredLightingRenderPass.hpp"

namespace PathFinder
{

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {}

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::DeferredLighting, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<uint32_t>(0, 0); // 1 uint Total number of lights
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Light table
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // Material table
        });

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

        DeferredLightingCBContent cbContent{}; 
        cbContent.GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent.GBufferDepthTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferDepthStencil);
        cbContent.OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        Scene* scene = context->GetContent()->GetScene();
        SceneGPUStorage* sceneGPUStorage = context->GetContent()->GetSceneGPUStorage();

        context->GetCommandRecorder()->BindExternalBuffer(*sceneGPUStorage->LightTable(), 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*sceneGPUStorage->MaterialTable(), 1, 0, HAL::ShaderRegister::ShaderResource);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);

        uint32_t lightCount = scene->FlatLights().size() + scene->SphericalLights().size();

        context->GetCommandRecorder()->SetRootConstants(lightCount, 0, 0);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
