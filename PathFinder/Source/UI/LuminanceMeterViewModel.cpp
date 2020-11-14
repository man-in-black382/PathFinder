#include "LuminanceMeterViewModel.hpp"

#include <RenderPipeline/RenderPasses/PipelineNames.hpp>

namespace PathFinder
{

    void LuminanceMeterViewModel::Import()
    {
        mLuminanceMeter = &Dependencies->ScenePtr->LumMeter();
    }

    void LuminanceMeterViewModel::Export()
    {
    }

    void LuminanceMeterViewModel::OnCreated()
    {
        (*Dependencies->PostRenderEvent) += { "LuminanceMeterViewModel.Post.Render", [this]()
        {
            const Memory::Buffer* histogram = Dependencies->ResourceStorage->GetPerResourceData(ResourceNames::LuminanceHistogram)->Buffer.get();

            histogram->Read<uint32_t>([this](const uint32_t* data)
            {
                if (data && mLuminanceMeter)
                    mLuminanceMeter->SetHistogramData(data);
            });
        }};
    }

}
