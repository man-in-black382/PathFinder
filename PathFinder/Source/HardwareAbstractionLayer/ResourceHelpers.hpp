#pragma once

#include <wrl.h>
#include <d3d12.h>

#include "ResourceState.hpp"

namespace HAL
{
    template <class ...States>
    D3D12_RESOURCE_STATES D3DResourceStatesFromVariantList(std::initializer_list<std::variant<States...>> states) {
        D3D12_RESOURCE_STATES d3dStates;
        for (auto state : states) {
            d3dStates |= D3DResourceStatesFromVariant(state);
        }
        return d3dStates;
    }

    template <class ...States>
    D3D12_RESOURCE_STATES D3DResourceStatesFromVariant(std::variant<States...> state) {
        D3D12_RESOURCE_STATES d3dStates;
        std::visit([&d3dStates](auto&& s) {
            d3dStates |= s.D3DState();
        }, state);
        return d3dStates;
    }
}

