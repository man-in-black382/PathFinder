#include "DeferredLightingRenderPass.hpp"

namespace PathFinder
{

    glm::vec3 VoxelWallIntersection(glm::vec3 voxelUVW, glm::vec3 voxelGridResolution, glm::vec3 rayDir)
    {
        static const float Epsilon = 0.00001;

        glm::vec3 s = glm::floor(voxelUVW * voxelGridResolution);
        glm::vec3 signV = glm::sign(rayDir);
        glm::vec3 u = (signV + 1.0f) / 2.0f;
        glm::vec3 w = (s + u) / voxelGridResolution;
        glm::vec3 a = (w - voxelUVW) / (rayDir + Epsilon); // Add e to prevent division by zero

        float d = a[0];
        for (auto i = 1; i < 3; ++i)
        {
            if ((a[i] < d && a[i] != 0) || d == 0)
            {
                d = a[i];
            }
        }

        return voxelUVW + rayDir * (d + Epsilon);
    }

    DeferredLightingRenderPass::DeferredLightingRenderPass()
        : RenderPass("DeferredLighting") {}

    void DeferredLightingRenderPass::SetupPipelineStates(PipelineStateCreator* stateCreator)
    {
        stateCreator->CreateComputeState(PSONames::DeferredLighting, [](ComputeStateProxy& state)
        {
            state.ShaderFileNames.ComputeShaderFileName = L"DeferredLightingRenderPass.hlsl";
        });
    }
      
    void DeferredLightingRenderPass::ScheduleResources(ResourceScheduler* scheduler)
    { 
        scheduler->NewTexture(ResourceNames::DeferredLightingOutput);
        scheduler->ReadTexture(ResourceNames::GBufferRT0);
        scheduler->ReadTexture(ResourceNames::GBufferDepthStencil);
        scheduler->WillUseRootConstantBuffer<DeferredLightingCBContent>();
    }  

    void DeferredLightingRenderPass::Render(RenderContext* context)
    {
        auto a = VoxelWallIntersection(glm::vec3(0.5, 0.5, 0.5), glm::vec3(3, 3, 3), glm::normalize(glm::vec3(-1.0, 0.2, 0.0)));

        context->GetCommandRecorder()->ApplyPipelineState(PSONames::DeferredLighting);

        auto cbContent = context->GetConstantsUpdater()->UpdateRootConstantBuffer<DeferredLightingCBContent>();
        cbContent->GBufferMaterialDataTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferRT0);
        cbContent->GBufferDepthTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::GBufferDepthStencil);
        cbContent->OutputTextureIndex = context->GetResourceProvider()->GetTextureDescriptorTableIndex(ResourceNames::DeferredLightingOutput);

        auto dimensions = context->GetDefaultRenderSurfaceDesc().DispatchDimensionsForGroupSize(32, 32);
        context->GetCommandRecorder()->Dispatch(dimensions.x, dimensions.y);
    }

}
