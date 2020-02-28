#include "DeferredLightingRenderPass.hpp"

namespace PathFinder
{

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {}

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature deferedLightingSinature = stateCreator->CloneBaseRootSignature();
        deferedLightingSinature.AddConstantsParameter(HAL::RootConstantsParameter{ 1, 0, 0 }); // 1 uint TOtal number of lights
        deferedLightingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 0, 0 }); // Light table
        deferedLightingSinature.AddDescriptorParameter(HAL::RootShaderResourceParameter{ 1, 0 }); // Material table
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
