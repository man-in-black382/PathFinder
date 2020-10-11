#include "DisplacementDistanceMapRenderPass.hpp"
#include <Scene/MaterialLoader.hpp>

namespace PathFinder
{

    DisplacementDistanceMapRenderPass::DisplacementDistanceMapRenderPass()
        : RenderPass("DistanceMapGeneration", RenderPassPurpose::AssetProcessing) {}

    void DisplacementDistanceMapRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator, RootSignatureCreator* rootSignatureCreator)
    {
        //HAL::RootSignature signature = stateCreator->CloneBaseRootSignature();
        //signature.AddDescriptorParameter(HAL::RootUnorderedAccessParameter{ 0, 0 }); // Read Only Cones Buffer | u0 - s0
        //signature.AddDescriptorParameter(HAL::RootUnorderedAccessParameter{ 1, 0 }); // Write Only Cones Buffer | u1 - s0

        //stateCreator->StoreRootSignature(RootSignatureNames::DisplacementDistanceMapGeneration, std::move(signature));

        //stateCreator->CreateComputeState(PSONames::DistanceMapHelperInitialization, [](ComputeStateProxy& state)
        //{
        //    state.ShaderFileNames.ComputeShaderFileName = L"DistanceMapHelperInitialization.hlsl";
        //    state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        //});

        //stateCreator->CreateComputeState(PSONames::DistanceMapGeneration, [](ComputeStateProxy& state)
        //{
        //    state.ShaderFileNames.ComputeShaderFileName = L"DistanceMapGeneration.hlsl";
        //    state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        //});

        //stateCreator->CreateComputeState(PSONames::DistanceMapHelperCompression, [](ComputeStateProxy& state)
        //{
        //    state.ShaderFileNames.ComputeShaderFileName = L"DistanceMapHelperCompression.hlsl";
        //    state.RootSignatureName = RootSignatureNames::DisplacementDistanceMapGeneration;
        //});
    }
     
    void DisplacementDistanceMapRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        ResourceScheduler::NewBufferProperties<JFACones> JFAConesBufferProps{
            MaterialLoader::DistanceFieldTextureSize.Width * 
            MaterialLoader::DistanceFieldTextureSize.Height *
            MaterialLoader::DistanceFieldTextureSize.Depth,
            1
        };

        scheduler->NewBuffer(ResourceNames::JumpFloodingCones0, JFAConesBufferProps);
        scheduler->NewBuffer(ResourceNames::JumpFloodingCones1, JFAConesBufferProps);
    } 

    void DisplacementDistanceMapRenderPass::Render(RenderContext<RenderPassContentMediator>* context)
    {
//        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DisplacementDistanceMapGenerationCBContent>();
//
//        ResourceProvider* resourceProvider = context->GetResourceProvider();
//        GPUCommandRecorder* commandRecorder = context->GetCommandRecorder();
//
//        ResourceName readOnlyConesBufferName = ResourceNames::JumpFloodingCones0;
//        ResourceName writeOnlyConesBufferName = ResourceNames::JumpFloodingCones1;
//
//        context->GetScene()->IterateMaterials([&](const Material& material)
//        {
//            /*if (material.DistanceAtlasIndirectionMap && material.DistanceAtlas)
//            {
//                return;
//            }*/
//
//            float dispatchX = ceilf((float)material.DistanceField->Dimensions().Width / 8);
//            float dispatchY = ceilf((float)material.DistanceField->Dimensions().Height / 8);
//            float dispatchZ = ceilf((float)material.DistanceField->Dimensions().Depth / 8);
//
//            // Setup common CB data
//          /*  cbContent->DisplacementMapSRVIndex = 
//                resourceProvider->GetExternalTextureDescriptorTableIndex(material.DisplacementMap, HAL::ShaderRegister::ShaderResource);
//
//            cbContent->DistanceAltasIndirectionMapUAVIndex = 
//                resourceProvider->GetExternalTextureDescriptorTableIndex(material.DistanceAtlasIndirectionMap, HAL::ShaderRegister::UnorderedAccess);
//
//            cbContent->DistanceAltasUAVIndex =
//                resourceProvider->GetExternalTextureDescriptorTableIndex(material.DistanceAtlas, HAL::ShaderRegister::UnorderedAccess);
//*/
//            cbContent->DisplacementMapSize = glm::uvec4{
//                material.DisplacementMap->Dimensions().Width,
//                material.DisplacementMap->Dimensions().Height,
//                0.0, 0.0
//            };
//
//            cbContent->DistanceAtlasIndirectionMapSize = glm::uvec4{
//                material.DistanceField->Dimensions().Width,
//                material.DistanceField->Dimensions().Height,
//                material.DistanceField->Dimensions().Depth,
//                0.0
//            };
//
//            // Initialize jump flooding helper texture
//            commandRecorder->ApplyPipelineState(PSONames::DistanceMapHelperInitialization);
//            commandRecorder->BindBuffer(readOnlyConesBufferName, 0, 0, HAL::ShaderRegister::UnorderedAccess);
//            commandRecorder->BindBuffer(writeOnlyConesBufferName, 1, 0, HAL::ShaderRegister::UnorderedAccess);
//            commandRecorder->Dispatch(dispatchX, dispatchY, dispatchZ);
//
//            uint64_t largestDimension = material.DistanceField->Dimensions().LargestDimension();
//            uint32_t jumpFloodingStepCount = log2(largestDimension);
//            uint32_t step = largestDimension / 2;
//
//            // Perform jump flooding (distance field computation)
//            commandRecorder->ApplyPipelineState(PSONames::DistanceMapGeneration);
//
//            // JFA + 4 algorithm, hence the jumpFloodingStepCount + 2
//            // JFA + 4 reduces errors to an acceptable minimum.
//            // Lower values were not acceptable in some cases
//            for (auto i = 0u; i < jumpFloodingStepCount + 4; ++i)
//            {
//                if (i >= jumpFloodingStepCount)
//                {
//                    step = 1;
//                }
//
//                cbContent->FloodStep = step;
//
//                commandRecorder->BindBuffer(readOnlyConesBufferName, 0, 0, HAL::ShaderRegister::UnorderedAccess);
//                commandRecorder->BindBuffer(writeOnlyConesBufferName, 1, 0, HAL::ShaderRegister::UnorderedAccess);
//                commandRecorder->Dispatch(dispatchX, dispatchY, dispatchZ);
//
//                std::swap(readOnlyConesBufferName, writeOnlyConesBufferName);
//            }
//
//            commandRecorder->ApplyPipelineState(PSONames::DistanceMapHelperCompression);
//            commandRecorder->BindBuffer(readOnlyConesBufferName, 0, 0, HAL::ShaderRegister::UnorderedAccess);
//            commandRecorder->BindBuffer(writeOnlyConesBufferName, 1, 0, HAL::ShaderRegister::UnorderedAccess);
//            commandRecorder->Dispatch(dispatchX, dispatchY, dispatchZ);
//        });
    }

}
