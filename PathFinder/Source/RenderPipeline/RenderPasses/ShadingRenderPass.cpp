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
        scheduler->NewTexture(ResourceNames::StochasticShadowedShadingOutput);
        scheduler->NewTexture(ResourceNames::StochasticUnshadowedShadingOutput);
        
        scheduler->ReadTexture(ResourceNames::GBufferAlbedoMetalness);
        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferTypeAndMaterialIndex);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);

        scheduler->UseRayTracing();
    } 

    void ShadingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::Shading);

        const Scene* scene = context->GetContent()->GetScene();
        const SceneGPUStorage* sceneStorage = context->GetContent()->GetSceneGPUStorage();
        const Memory::Texture* blueNoiseTexture = scene->BlueNoiseTexture();

        auto resourceProvider = context->GetResourceProvider();

        ShadingCBContent cbContent{};

        cbContent.GBufferIndices.AlbedoMetalnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferAlbedoMetalness);
        cbContent.GBufferIndices.NormalRoughnessTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferNormalRoughness);
        cbContent.GBufferIndices.MotionTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferMotionVector);
        cbContent.GBufferIndices.TypeAndMaterialTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferTypeAndMaterialIndex);
        cbContent.GBufferIndices.DepthStencilTexIdx = resourceProvider->GetSRTextureIndex(ResourceNames::GBufferDepthStencil);
        cbContent.BlueNoiseTexIdx = blueNoiseTexture->GetSRDescriptor()->IndexInHeapRange();
        cbContent.AnalyticOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::ShadingAnalyticOutput);
        cbContent.StochasticShadowedOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticShadowedShadingOutput);
        cbContent.StochasticUnshadowedOutputTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::StochasticUnshadowedShadingOutput);
        cbContent.BlueNoiseTextureSize = { blueNoiseTexture->Properties().Dimensions.Width, blueNoiseTexture->Properties().Dimensions.Height };

        auto frameNumber = context->FrameNumber();
        auto startIndex = frameNumber * ShadingCBContent::MaxSupportedLights;
        auto endIndex = startIndex + ShadingCBContent::MaxSupportedLights - 1;

        auto haltonSequence = Foundation::Halton::Sequence<4>(startIndex, endIndex);

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
