#include "ShadingRenderPass.hpp"

#include "../Foundation/Halton.hpp"

namespace PathFinder
{

    ShadingRenderPass::ShadingRenderPass()
        : RenderPass("Shading") {}

    void ShadingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        stateCreator->CreateRayTracingState(PSONames::Shadows, [](RayTracingStateProxy& state)
        {
            state.RayGenerationShaderFileName = "ShadingRenderPass.hlsl";
            state.AddMissShader("ShadingRenderPass.hlsl");
            state.ShaderConfig = HAL::RayTracingShaderConfig{ sizeof(float), 0 };
            state.GlobalRootSignatureName = RootSignatureNames::RayTracing;
            state.PipelineConfig = HAL::RayTracingPipelineConfig{ 2 };
        });
    }
     
    void ShadingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::AnalyticalLuminanceOutput);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
    } 

    void ShadingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Shadows);

        const Scene* scene = context->GetContent()->GetScene();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const Memory::Texture* blueNoiseTexture = scene->BlueNoiseTexture();

        ShadingCBContent cbContent{};
        cbContent.BlueNoiseTextureIndex = blueNoiseTexture->GetSRDescriptor()->IndexInHeapRange();
        cbContent.AnalyticalLuminanceOutputTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::AnalyticalLuminanceOutput);
        cbContent.GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GBufferRT0);
        cbContent.GBufferDepthTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.BlueNoiseTextureSize = { blueNoiseTexture->Properties().Dimensions.Width, blueNoiseTexture->Properties().Dimensions.Height };
        cbContent.LightOffsets = sceneStorage->LightTablePartitionInfo();
        
        uint32_t startIndex = mFrameNumber * (ShadingCBContent::MaxSupportedLights - 1);
        uint32_t endIndex = startIndex + (ShadingCBContent::MaxSupportedLights - 1);

        auto haltonSequence = Foundation::Halton::Sequence<4>(startIndex, endIndex);

        for (auto i = 0; i < haltonSequence.size(); ++i)
        {
            for (auto j = 0; j < haltonSequence[i].size(); ++j)
            {
                cbContent.HaltonSequence[i][j] = haltonSequence[i][j];
            }
        }

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);

        const Memory::Buffer* bvh = sceneStorage->TopAccelerationStructure().AccelerationStructureBuffer();
        const Memory::Buffer* lights = sceneStorage->LightTable();
        const Memory::Buffer* materials = sceneStorage->MaterialTable();

        context->GetCommandRecorder()->BindExternalBuffer(*bvh, 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*lights, 4, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*materials, 5, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->DispatchRays(context->GetDefaultRenderSurfaceDesc().Dimensions());

        mFrameNumber++;
    }

}
