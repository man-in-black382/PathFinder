#include "SamplerCreator.hpp"

namespace PathFinder
{

    SamplerCreator::SamplerCreator(PipelineResourceStorage* storage)
        : mResourceStorage{ storage } {}

    void SamplerCreator::CreateSampler(Foundation::Name name, const HAL::Sampler& sampler)
    {
        mResourceStorage->AddSampler(name, sampler);
    }

}
