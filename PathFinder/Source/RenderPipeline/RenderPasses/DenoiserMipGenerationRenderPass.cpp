#include "DenoiserMipGenerationRenderPass.hpp"
#include "ResourceNameResolving.hpp"

#include <Foundation/Gaussian.hpp>

#include <array>
#include <vector>

namespace PathFinder
{

    DenoiserMipGenerationRenderPass::DenoiserMipGenerationRenderPass()
        : RenderPass("DenoiserMipGeneration") {}

    void DenoiserMipGenerationRenderPass::ScheduleSubPasses(SubPassScheduler<RenderPassContentMediator>* scheduler)
    {
        auto frameIndex = scheduler->FrameNumber() % 2;
        bool isDenoiserEnabled = scheduler->Content()->GetSettings()->IsDenoiserEnabled;

        std::array<std::vector<DownsamplingInvocationInputs>, 4> perResourceDownsamplingInvocationInputs;

        if (isDenoiserEnabled)
        {
            perResourceDownsamplingInvocationInputs[0] = GenerateDownsamplingShaderInvocationInputs(
                ResourceNames::GBufferViewDepth[frameIndex],
                scheduler->GetTextureProperties(ResourceNames::GBufferViewDepth[frameIndex]),
                DownsamplingCBContent::Filter::Min,
                DownsamplingStrategy::WriteAllLevels);

            perResourceDownsamplingInvocationInputs[1] = GenerateDownsamplingShaderInvocationInputs(
                DenoiserMipGenerationShadowedInputTexName(false, frameIndex),
                scheduler->GetTextureProperties(DenoiserMipGenerationShadowedInputTexName(false, frameIndex)),
                DownsamplingCBContent::Filter::Average,
                DownsamplingStrategy::WriteAllLevels);

            perResourceDownsamplingInvocationInputs[2] = GenerateDownsamplingShaderInvocationInputs(
                DenoiserMipGenerationUnshadowedInputTexName(false),
                scheduler->GetTextureProperties(DenoiserMipGenerationUnshadowedInputTexName(false)),
                DownsamplingCBContent::Filter::Average,
                DownsamplingStrategy::WriteAllLevels);
        };

        auto longestInvocationArrayIt = std::max_element(
            perResourceDownsamplingInvocationInputs.begin(),
            perResourceDownsamplingInvocationInputs.end(),
            [](auto& first, auto& second) -> bool { return first.size() < second.size(); });

        for (auto invocation = 0; invocation < longestInvocationArrayIt->size(); ++invocation)
        {
            if (mDownsamplingSubPasses.size() <= invocation)
            {
                mDownsamplingSubPasses.emplace_back(std::make_unique<DownsamplingRenderSubPass>("DenoiserMipGeneration", invocation));
            }

            std::vector<DownsamplingInvocationInputs> subPassInvocationInputs;

            for (const std::vector<DownsamplingInvocationInputs>& inputs : perResourceDownsamplingInvocationInputs)
            {
                if (invocation < inputs.size())
                {
                    subPassInvocationInputs.push_back(inputs[invocation]);
                }
            }

            DownsamplingRenderSubPass* subPass = mDownsamplingSubPasses[invocation].get();
            subPass->SetInvocationInputs(subPassInvocationInputs);
            scheduler->AddRenderSubPass(subPass);
        }

        for (auto& subPass : mDownsamplingSubPasses)
        {
            subPass->SetEnabled(scheduler->Content()->GetSettings()->IsDenoiserEnabled);
        }
    }

}
