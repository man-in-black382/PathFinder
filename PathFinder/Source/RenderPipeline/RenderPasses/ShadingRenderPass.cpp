#include "ShadingRenderPass.hpp"

#include "../Foundation/Halton.hpp"

namespace PathFinder
{

    ShadingRenderPass::ShadingRenderPass()
        : RenderPass("Shading") {} 

    void ShadingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::RayTracing, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<uint32_t>(0, 0);
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Scene BVH | t0 - s0
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // Light Table | t1 - s0
            signatureProxy.AddShaderResourceBufferParameter(2, 0); // Material Table | t2 - s0
        });

        stateCreator->CreateRayTracingState(PSONames::Shading, [this](RayTracingStateProxy& state)
        {
            state.RayGenerationShaderFileName = "Shading.hlsl";
            state.AddMissShader("Shading.hlsl");
            state.ShaderConfig = HAL::RayTracingShaderConfig{ sizeof(float), sizeof(float) * 2 };
            state.GlobalRootSignatureName = RootSignatureNames::RayTracing;
            state.PipelineConfig = HAL::RayTracingPipelineConfig{ 1 };
        });
    }
     
    void ShadingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::ShadingAnalyticOutput);
        scheduler->NewTexture(ResourceNames::ShadingStochasticShadowedOutput);
        scheduler->NewTexture(ResourceNames::ShadingStochasticUnshadowedOutput);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture({ ResourceNames::GBufferDepthStencil, scheduler->FrameNumber() % 2 });
    } 

    void ShadingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Shading);

        const Scene* scene = context->GetContent()->GetScene();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const Memory::Texture* blueNoiseTexture = scene->BlueNoiseTexture();

        ShadingCBContent cbContent{};
        cbContent.BlueNoiseTextureIndex = blueNoiseTexture->GetSRDescriptor()->IndexInHeapRange();
        cbContent.AnalyticOutputTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ShadingAnalyticOutput);
        cbContent.StochasticShadowedOutputTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ShadingStochasticShadowedOutput);
        cbContent.StochasticUnshadowedOutputTextureIndex = context->GetResourceProvider()->GetUATextureIndex(ResourceNames::ShadingStochasticUnshadowedOutput);
        cbContent.GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetSRTextureIndex(ResourceNames::GBufferRT0);
        cbContent.GBufferDepthTextureIndex = context->GetResourceProvider()->GetSRTextureIndex({ ResourceNames::GBufferDepthStencil, context->FrameNumber() % 2 });
        cbContent.BlueNoiseTextureSize = { blueNoiseTexture->Properties().Dimensions.Width, blueNoiseTexture->Properties().Dimensions.Height };
        cbContent.LightOffsets = sceneStorage->LightTablePartitionInfo();

        auto haltonSequence = Foundation::Halton::Sequence<4>(0, ShadingCBContent::MaxSupportedLights - 1);

        for (auto i = 0; i < haltonSequence.size(); ++i)
        {
            for (auto j = 0; j < haltonSequence[i].size(); ++j)
            {
                cbContent.HaltonSequence[i][j] = haltonSequence[i][j];
            }
        }

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRootConstants(CompressLightPartitionInfo(sceneStorage->LightTablePartitionInfo()), 0, 0);

        const Memory::Buffer* bvh = sceneStorage->TopAccelerationStructure().AccelerationStructureBuffer();
        const Memory::Buffer* lights = sceneStorage->LightTable();
        const Memory::Buffer* materials = sceneStorage->MaterialTable();

        context->GetCommandRecorder()->BindExternalBuffer(*bvh, 0, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*lights, 1, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->BindExternalBuffer(*materials, 2, 0, HAL::ShaderRegister::ShaderResource);
        context->GetCommandRecorder()->DispatchRays(context->GetDefaultRenderSurfaceDesc().Dimensions());
    }

    uint32_t ShadingRenderPass::CompressLightPartitionInfo(const GPULightTablePartitionInfo& info) const
    {
        uint32_t compressed = 0;
        compressed |= (info.SphericalLightsOffset & 0xFF) << 24;
        compressed |= (info.RectangularLightsOffset & 0xFF) << 16;
        compressed |= (info.EllipticalLightsOffset & 0xFF) << 8;
        compressed |= (info.TotalLightsCount & 0xFF);
        return compressed;
    }

}
