#pragma once

#include "PipelineResourceStorage.hpp"

namespace PathFinder
{

    class RootConstantsUpdater
    {
    public:
        RootConstantsUpdater(PipelineResourceStorage* storage);

        template <class RootCBufferContent>
        void UpdateRootConstantBuffer(const RootCBufferContent& data);

    private:
        PipelineResourceStorage* mResourceStorage;
    };

    template <class RootCBufferContent>
    void RootConstantsUpdater::UpdateRootConstantBuffer(const RootCBufferContent& data)
    {
        return mResourceStorage->UpdateCurrentPassRootConstants(data);
    }

}
