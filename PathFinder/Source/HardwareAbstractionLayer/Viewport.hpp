#pragma once

#include <cstdint>
#include <d3d12.h>

namespace HAL
{
    class Viewport
    {
    public:
        Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float minDepth, float maxDepth);
        Viewport(uint32_t width, uint32_t height, float minDepth, float maxDepth);
        Viewport(uint32_t width, uint32_t height);

        D3D12_VIEWPORT D3DViewport() const;

        uint32_t X;
        uint32_t Y;
        uint32_t Width;
        uint32_t Height;

        float MinDepth;
        float MaxDepth;
    };
}

