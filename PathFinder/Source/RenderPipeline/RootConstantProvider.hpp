#pragma once

#include "ResourceStorage.hpp"

namespace PathFinder
{

    class RootConstantsProvider
    {
    public:
        RootConstantsProvider(ResourceStorage* storage);

        template <class RootCBufferContent>
        RootCBufferContent* UpdateRootConstantBuffer();

    private:
        ResourceStorage* mResourceStorage;
    };

    template <class RootCBufferContent>
    RootCBufferContent* RootConstantsProvider::UpdateRootConstantBuffer()
    {
        // 
    }

}
