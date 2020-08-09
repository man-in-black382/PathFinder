#pragma once

#include "../PipelineResourceStorage.hpp"

namespace PathFinder
{

    class SamplerCreator
    {
    public:
        SamplerCreator(PipelineResourceStorage* storage);

        void CreateSampler(Foundation::Name name, const HAL::Sampler& sampler);

    private:
        PipelineResourceStorage* mResourceStorage;
    };

}
