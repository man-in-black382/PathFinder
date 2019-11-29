#include "DisplacementDistanceMapRenderPass.hpp"

namespace PathFinder
{

    DisplacementDistanceMapRenderPass::DisplacementDistanceMapRenderPass()
        : RenderPass("DistanceMapGeneration", Purpose::AssetProcessing) {}

    void DisplacementDistanceMapRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        HAL::RootSignature signature = stateCreator->CloneBaseRootSignature();
        signature.AddDescriptorParameter(HAL::RootUnorderedAccessParameter{ 0, 0 }); // UAV Counter | u0 - s0

        stateCreator->StoreRootSignature(RootSignatureNames::DisplacementDistanceMapGeneration, std::move(signature));

        stateCreator->CreateComputeState(PSONames::DistanceMapHelperInitialization, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DistanceMapHelperInitialization.hlsl";
            state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        });

        stateCreator->CreateComputeState(PSONames::DistanceMapGeneration, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DistanceMapGeneration.hlsl";
            state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        });

        stateCreator->CreateComputeState(PSONames::DistanceMapHelperCompression, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DistanceMapHelperCompression.hlsl";
            state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        });
    }
     
    void DisplacementDistanceMapRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        ResourceScheduler::NewTextureProperties JFAConeIndirectionTextureProps{};
        JFAConeIndirectionTextureProps.Dimensions = { 128, 128, 64 };
        JFAConeIndirectionTextureProps.Kind = HAL::ResourceFormat::TextureKind::Texture3D;
        JFAConeIndirectionTextureProps.ShaderVisibleFormat = HAL::ResourceFormat::Color::RGBA32_Float;

        ResourceScheduler::NewTextureProperties JFAConesTextureProps{};
        // Depth 8 for eight cones
        JFAConesTextureProps.Dimensions = { 128, 128, 8 };
        JFAConesTextureProps.Kind = HAL::ResourceFormat::TextureKind::Texture3D;
        JFAConesTextureProps.ShaderVisibleFormat = HAL::ResourceFormat::Color::RGBA32_Float;

        // Ping-pong textures
        scheduler->NewTexture(ResourceNames::JumpFloodingConesIndirection0, JFAConeIndirectionTextureProps);
        scheduler->NewTexture(ResourceNames::JumpFloodingConesIndirection1, JFAConeIndirectionTextureProps);
        scheduler->NewTexture(ResourceNames::JumpFloodingCones0, JFAConesTextureProps);
        scheduler->NewTexture(ResourceNames::JumpFloodingCones1, JFAConesTextureProps);

        scheduler->WillUseRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();
    } 

    void DisplacementDistanceMapRenderPass::Render(RenderContext* context)
    {
        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();

        uint32_t conesIndirectionUAVIndex0 = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::JumpFloodingConesIndirection0);
        uint32_t conesIndirectionUAVIndex1 = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::JumpFloodingConesIndirection1);
        uint32_t conesUAVIndex0 = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::JumpFloodingCones0);
        uint32_t conesUAVIndex1 = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::JumpFloodingCones1);

        auto commandRecorder = context->GetCommandRecorder();

        ResourceName readOnlyJFAHelperName = ResourceNames::JumpFloodingConesIndirection0;
        ResourceName writeOnlyJFAHelperName = ResourceNames::JumpFloodingConesIndirection1;

        context->GetScene()->IterateMaterials([&](const Material& material)
        {
            /*if (material.DistanceAtlasIndirectionMap && material.DistanceAtlas)
            {
                return;
            }*/

            float dispatchX = ceilf((float)material.DistanceAtlasIndirectionMap->Dimensions().Width / 8);
            float dispatchY = ceilf((float)material.DistanceAtlasIndirectionMap->Dimensions().Height / 8);
            float dispatchZ = ceilf((float)material.DistanceAtlasIndirectionMap->Dimensions().Depth / 8);

            // Setup common CB data
            cbContent->DisplacementMapSRVIndex = 
                context->GetResourceProvider()->GetExternalTextureDescriptorTableIndex(material.DisplacementMap, HAL::ShaderRegister::ShaderResource);

            cbContent->DistanceAltasIndirectionMapUAVIndex = 
                context->GetResourceProvider()->GetExternalTextureDescriptorTableIndex(material.DistanceAtlasIndirectionMap, HAL::ShaderRegister::UnorderedAccess);

            cbContent->DistanceAltasUAVIndex =
                context->GetResourceProvider()->GetExternalTextureDescriptorTableIndex(material.DistanceAtlas, HAL::ShaderRegister::UnorderedAccess);

            cbContent->DisplacementMapSize = glm::uvec4{
                material.DisplacementMap->Dimensions().Width,
                material.DisplacementMap->Dimensions().Height,
                0.0, 0.0
            };

            cbContent->DistanceAtlasIndirectionMapSize = glm::uvec4{
                material.DistanceAtlasIndirectionMap->Dimensions().Width,
                material.DistanceAtlasIndirectionMap->Dimensions().Height,
                material.DistanceAtlasIndirectionMap->Dimensions().Depth,
                0.0
            };

            // Initialize jump flooding helper texture
            commandRecorder->ApplyPipelineState(PSONames::DistanceMapHelperInitialization);
            commandRecorder->Dispatch(dispatchX, dispatchY, dispatchZ);
            commandRecorder->WaitUntilUnorderedAccessesComplete(readOnlyJFAHelperName);
            commandRecorder->WaitUntilUnorderedAccessesComplete(writeOnlyJFAHelperName);

            uint64_t largestDimension = material.DistanceAtlasIndirectionMap->Dimensions().LargestDimension();
            uint32_t jumpFloodingStepCount = log2(largestDimension);
            uint32_t step = largestDimension / 2;

            // Perform jump flooding (distance field computation)
            commandRecorder->ApplyPipelineState(PSONames::DistanceMapGeneration);

            // JFA + 1 algorithm, hence the jumpFloodingStepCount + 1
            for (auto i = 0u; i < jumpFloodingStepCount + 1; ++i)
            {
                if (i >= jumpFloodingStepCount)
                {
                    step = 1;
                }

                cbContent->FloodStep = step;

                cbContent->ReadOnlyJFAConesIndirectionUAVIndex = conesIndirectionUAVIndex0;
                cbContent->WriteOnlyJFAConesIndirectionUAVIndex = conesIndirectionUAVIndex1;

                commandRecorder->Dispatch(dispatchX, dispatchY, dispatchZ);
                commandRecorder->WaitUntilUnorderedAccessesComplete(readOnlyJFAHelperName);
                commandRecorder->WaitUntilUnorderedAccessesComplete(writeOnlyJFAHelperName);

                std::swap(conesIndirectionUAVIndex0, conesIndirectionUAVIndex1);
                std::swap(readOnlyJFAHelperName, writeOnlyJFAHelperName);
            }

            // Transfer helper texture data to asset texture and compress it at the same time
            cbContent->ReadOnlyJFAConesIndirectionUAVIndex = conesIndirectionUAVIndex0;

            commandRecorder->ApplyPipelineState(PSONames::DistanceMapHelperCompression);
            commandRecorder->Dispatch(dispatchX, dispatchY, dispatchZ);
            commandRecorder->WaitUntilUnorderedAccessesComplete(readOnlyJFAHelperName);
            commandRecorder->WaitUntilUnorderedAccessesComplete(writeOnlyJFAHelperName);
        });
    }

}
