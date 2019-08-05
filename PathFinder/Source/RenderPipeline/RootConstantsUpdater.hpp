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
        if (RootCBufferData* data = mResourceStorage->GetRootConstantDataForCurrentPass())
        {
            return data;
        }
        else {
            throw std::runtime_error("Root constant buffer was not scheduled for use for this pass");
        }
    }

}
