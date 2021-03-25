#include "DeferredLightingRenderPass.hpp"
#include "ResourceNameResolving.hpp"

#include <Foundation/Halton.hpp>

namespace PathFinder
{

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {} 

    void DeferredLightingRenderPass::SetupRootSignatures(RootSignatureCreator* rootSignatureCreator)
    {
        rootSignatureCreator->CreateRootSignature(RootSignatureNames::ShadingCommon, [](RootSignatureProxy& signatureProxy)
        {
            signatureProxy.AddRootConstantsParameter<uint32_t>(0, 0);
            signatureProxy.AddShaderResourceBufferParameter(0, 0); // Scene BVH | t0 - s0
            signatureProxy.AddShaderResourceBufferParameter(1, 0); // Light Table | t1 - s0
            signatureProxy.AddShaderResourceBufferParameter(2, 0); // Material Table | t2 - s0
            signatureProxy.AddShaderResourceBufferParameter(3, 0); // Vertex Buffer | t3 - s0
            signatureProxy.AddShaderResourceBufferParameter(4, 0); // Index Buffer | t4 - s0
            signatureProxy.AddShaderResourceBufferParameter(5, 0); // Mesh Instance Table | t5 - s0
        });
    }

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DeferredLighting, [this](ComputeStateProxy& state)
        {
            state.ComputeShaderFileName = "DeferredLighting.hlsl";
            state.RootSignatureName = RootSignatureNames::ShadingCommon;
        });
    }
     
    void DeferredLightingRenderPass::ScheduleResources(ResourceScheduler<RenderPassContentMediator>* scheduler)
    { 
        bool isDenoiserEnabled = scheduler->Content()->GetSettings()->IsDenoiserEnabled;
        auto currentFrameIndex = scheduler->FrameNumber() % 2;

        //auto defaultDimensions = scheduler->DefaultRenderSurfaceDesc().Dimensions();
        //Geometry::Dimensions luminancesTextureDimensions{ defaultDimensions.Width, defaultDimensions.Height, 4 }; // 4 lights

        scheduler->NewTexture(ResourceNames::ShadingAnalyticOutput);
        scheduler->NewTexture(ResourceNames::DeferredLightingRayPDFs, NewTextureProperties{ HAL::ColorFormat::RGBA8_Usigned_Norm });
        scheduler->NewTexture(ResourceNames::DeferredLightingRayLightIntersectionPoints, NewTextureProperties{ HAL::ColorFormat::RGBA32_Unsigned });
        //scheduler->NewTexture(ResourceNames::DeferredLightingRayLuminances, NewTextureProperties{ HAL::ColorFormat::R11G11B10_Float, HAL::TextureKind::Texture3D, luminancesTextureDimensions });
        
        scheduler->ReadTexture(ResourceNames::GBufferAlbedoMetalness);
        scheduler->ReadTexture(ResourceNames::GBufferNormalRoughness);
        scheduler->ReadTexture(ResourceNames::GBufferMotionVector);
        scheduler->ReadTexture(ResourceNames::GBufferTypeAndMaterialIndex);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->ReadTexture(DeferredLightingRngSeedTexName(isDenoiserEnabled, currentFrameIndex));
    } 

    void DeferredLightingRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);

        bool isDenoiserEnabled = context->GetContent()->GetSettings()->IsDenoiserEnabled;
        auto currentFrameIndex = context->FrameNumber() % 2;

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
        cbContent.ShadowRayPDFsTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingRayPDFs);
        cbContent.ShadowRayIntersectionPointsTexIdx = resourceProvider->GetUATextureIndex(ResourceNames::DeferredLightingRayLightIntersectionPoints);
        cbContent.BlueNoiseTextureSize = { blueNoiseTexture->Properties().Dimensions.Width, blueNoiseTexture->Properties().Dimensions.Height };
        cbContent.RngSeedsTexIdx = resourceProvider->GetSRTextureIndex(DeferredLightingRngSeedTexName(isDenoiserEnabled, currentFrameIndex));
        cbContent.FrameNumber = context->FrameNumber();

        auto haltonSequence = Foundation::Halton::Sequence(0, 3);

        for (auto i = 0; i < 4; ++i)
        {
            cbContent.Halton[i] = haltonSequence[i];
        }

        context->GetConstantsUpdater()->UpdateRootConstantBuffer(cbContent);
        context->GetCommandRecorder()->SetRootConstants(sceneStorage->CompressedLightPartitionInfo(), 0, 0);

        const Memory::Buffer* bvh = sceneStorage->TopAccelerationStructure().AccelerationStructureBuffer();
        const Memory::Buffer* lights = sceneStorage->LightTable();
        const Memory::Buffer* materials = sceneStorage->MaterialTable();

        if (bvh) context->GetCommandRecorder()->BindExternalBuffer(*bvh, 0, 0, HAL::ShaderRegister::ShaderResource);
        if (lights) context->GetCommandRecorder()->BindExternalBuffer(*lights, 1, 0, HAL::ShaderRegister::ShaderResource);
        if (materials) context->GetCommandRecorder()->BindExternalBuffer(*materials, 2, 0, HAL::ShaderRegister::ShaderResource);
        
        context->GetCommandRecorder()->Dispatch(context->GetDefaultRenderSurfaceDesc().Dimensions(), { 8, 8 });
    }

}
