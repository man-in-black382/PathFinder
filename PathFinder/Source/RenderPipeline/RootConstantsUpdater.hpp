#pragma once

#include "PipelineResourceStorage.hpp"

namespace PathFinder
{

    class RootConstantsUpdater
    {
    public:
        RootConstantsUpdater(PipelineResourceStorage* storage);

        template <class RootCBufferContent>
        RootCBufferContent* UpdateRootConstantBuffer();

    private:
        PipelineResourceStorage* mResourceStorage;
    };

    template <class RootCBufferData>
    RootCBufferData* RootConstantsUpdater::UpdateRootConstantBuffer()
    {
        return mResourceStorage->RootConstantDataForCurrentPass<RootCBufferData>();
    }

}
