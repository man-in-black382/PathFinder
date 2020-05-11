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
        ResourceScheduler::NewTextureProperties outputProperties{};
        outputProperties.TextureCount = 2;
        outputProperties.MipCount = 5;

        scheduler->NewTexture(ResourceNames::ShadingAnalyticOutput);
        scheduler->NewTexture(ResourceNames::ShadingStochasticShadowedOutput, outputProperties);
        scheduler->NewTexture(ResourceNames::ShadingStochasticUnshadowedOutput, outputProperties);
        
        scheduler->ReadTexture(ResourceNames::GBufferAlbedoMetalness);
        scheduler->ReadTexture(ResourceNames::GBufferRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferNormal);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferTypeAndMaterialIndex);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
    } 

    void ShadingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Shading);

        const Scene* scene = context->GetContent()->GetScene();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const Memory::Texture* blueNoiseTexture = scene->BlueNoiseTexture();

        auto resourceProvider = context->GetResourceProvider();
        auto currentFrameIndex = context->FrameNumber() % 2;

        ShadingCBContent cbContent{};

        cbContent.GBufferIndices.AlbedoMetalnessTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferAlbedoMetalness);
        cbContent.GBufferIndices.RoughnessTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferRoughness);
        cbContent.GBufferIndices.NormalTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormal);
        cbContent.GBufferIndices.MotionTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.GBufferIndices.TypeAndMaterialTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferTypeAndMaterialIndex);
        cbContent.GBufferIndices.DepthStencilTextureIndex = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);

        cbContent.BlueNoiseTextureIndex = blueNoiseTexture->GetSRDescriptor()->IndexInHeapRange();
        cbContent.AnalyticOutputTextureIndex = resourceProvider->GetUATextureIndex(ResourceNames::ShadingAnalyticOutput);
        cbContent.StochasticShadowedOutputTextureIndex = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticShadowedOutput, currentFrameIndex });
        cbContent.StochasticUnshadowedOutputTextureIndex = resourceProvider->GetUATextureIndex({ ResourceNames::ShadingStochasticUnshadowedOutput, currentFrameIndex });
        cbContent.BlueNoiseTextureSize = { blueNoiseTexture->Properties().Dimensions.Width, blueNoiseTexture->Properties().Dimensions.Height };

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
