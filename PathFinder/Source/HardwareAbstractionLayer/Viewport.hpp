#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
    class Viewport
    {
    public:
        Viewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float minDepth, float maxDepth);
        Viewport(uint16_t width, uint16_t height, float minDepth, float maxDepth);
        Viewport(uint16_t width, uint16_t height);

        D3D12_VIEWPORT D3DViewport() const;

        uint16_t X;
        uint16_t Y;
        uint16_t Width;
        uint16_t Height;

        float MinDepth;
        float MaxDepth;
    };
}

