#pragma once

#include "ResourceStorage.hpp"

namespace PathFinder
{

    class RootConstantsUpdater
    {
    public:
        RootConstantsUpdater(ResourceStorage* storage);

        template <class RootCBufferContent>
        RootCBufferContent* UpdateRootConstantBuffer();

    private:
        ResourceStorage* mResourceStorage;
    };

    template <class RootCBufferData>
    RootCBufferData* RootConstantsUpdater::UpdateRootConstantBuffer()
    {
        return mResourceStorage->GetRootConstantDataForCurrentPass<RootCBufferData>();
    }

}
