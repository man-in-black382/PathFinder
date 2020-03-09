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
            state.ComputeShaderFileName = "DeferredLightingRenderPass.hlsl";
            state.RootSignatureName = RootSignatureNames::DeferredLighting;
        });
    }
      
    void DeferredLightingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    {
        scheduler->NewTexture(ResourceNames::DeferredLightingFullOutput);
        scheduler->NewTexture(ResourceNames::DeferredLightingOverexposedOutput);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
    }  

    void DeferredLightingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);

        auto resourceProvider = context->GetResourceProvider();

        DeferredLightingCBContent cbContent{}; 
        cbContent.GBufferMaterialDataTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferRT0);
        cbContent.GBufferDepthTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.OutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingFullOutput);
        cbContent.OverexposedOutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingOverexposedOutput);

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
