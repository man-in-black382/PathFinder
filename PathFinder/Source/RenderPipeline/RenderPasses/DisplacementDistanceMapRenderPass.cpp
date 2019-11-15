#include "DisplacementDistanceMapRenderPass.hpp"

namespace PathFinder
{

    DisplacementDistanceMapRenderPass::DisplacementDistanceMapRenderPass()
        : RenderPass("DisplaycmentDistanceMapGeneration", Purpose::AssetProcessing) {}

    void DisplacementDistanceMapRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature signature = stateCreator->CloneBaseRootSignature();
        signature.AddDescriptorParameter(HAL::RootUnorderedAccessParameter{ 0, 0 }); // UAV Counter | u0 - s0

        stateCreator->StoreRootSignature(RootSignatureNames::DisplacementDistanceMapGeneration, std::move(signature));

        stateCreator->CreateComputeState(PSONames::DisplacementDistanceMapGeneration, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DisplacementDistanceMapGenerationRenderPass.hlsl";
            state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        });
    }
     
    void DisplacementDistanceMapRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->WillUseRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();
    } 

    void DisplacementDistanceMapRenderPass::Render(RenderContext* context)
    {
        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DisplacementDistanceMapGeneration);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();
        
        context->GetScene()->IterateMaterials([&](const Material& material)
        {
            if (material.DistanceAtlasIndirectionMap && material.DistanceAtlas)
            {
                return;
            }

            cbContent->DisplacementMapSRVIndex = 
                context->GetResourceProvider()->GetExternalTextureDescriptorTableIndex(material.DisplacementMap, HAL::ShaderRegister::ShaderResource);

            cbContent->DistanceAltasIndirectionMapUAVIndex = 
                context->GetResourceProvider()->GetExternalTextureDescriptorTableIndex(material.DistanceAtlasIndirectionMap, HAL::ShaderRegister::UnorderedAccess);

            cbContent->DistanceAltasUAVIndex =
                context->GetResourceProvider()->GetExternalTextureDescriptorTableIndex(material.DistanceAtlas, HAL::ShaderRegister::UnorderedAccess);

            auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
            context->GetCommandRecorder()->Dispatch(
                ceilf((float)material.DistanceAtlasIndirectionMap->Dimensions().Width / 8),
                ceilf((float)material.DistanceAtlasIndirectionMap->Dimensions().Height / 8),
                8);
        });

        
    }

}
