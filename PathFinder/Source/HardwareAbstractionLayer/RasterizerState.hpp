#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
   
    class RasterizerState {
    public:
        enum class FillMode { Wireframe, Solid };
        enum class CullMode { None, Back, Front };

        RasterizerState();

        void SetFillMode(FillMode mode);
        void SetCullMode(CullMode mode);
        void SetFrontClockwise(bool frontIsClockwise);

    private:
        D3D12_RASTERIZER_DESC mDesc{};

    public:
        inline const auto& D3DState() const { return mDesc; }
    };

}

