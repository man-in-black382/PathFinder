#pragma once

#include "RenderTarget.hpp"

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
   
    class BlendState {
    public:
        enum class Value {
            Zero, One, SourceColor, InverveSourceColor, SourceAlpha, InverseSourceAlpha,
            DestinationAlpha, InverseDestinationAlpha, DestinationColor, InverseDestinationColor 
        };

        enum class Function { Addition, Substraction, ReverseSubstraction, Min, Max };

        BlendState();

        void SetSourceValues(Value color, Value alpha, RenderTarget renderTarget = RenderTarget::RT0);
        void SetDestinationValues(Value color, Value alpha, RenderTarget renderTarget = RenderTarget::RT0);
        void SetFunctions(Function colorFunction, Function alphaFunction, RenderTarget renderTarget = RenderTarget::RT0);
        void SetBlendingEnabled(bool enabled, RenderTarget renderTarget = RenderTarget::RT0);

    private:
        D3D12_BLEND_DESC mDesc{};

        D3D12_BLEND D3DValue(Value value);

    public:
        inline const auto& D3DState() const { return mDesc; }
    };

}

