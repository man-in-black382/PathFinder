#include "Viewport.hpp"

namespace HAL
{

    Viewport::Viewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height, float minDepth, float maxDepth)
        : X(x), Y(y), Width(width), Height(height), MinDepth(minDepth), MaxDepth(maxDepth) {}

    Viewport::Viewport(uint16_t width, uint16_t height, float minDepth, float maxDepth)
        : Viewport(0, 0, width, height, minDepth, maxDepth) {}

    Viewport::Viewport(uint16_t width, uint16_t height)
        : Viewport(width, height, 0.0f, 1.0f) {}

    D3D12_VIEWPORT Viewport::D3DViewport() const
    {
        D3D12_VIEWPORT viewport;
        viewport.TopLeftX = X;
        viewport.TopLeftY = Y;
        viewport.Height = Height;
        viewport.Width = Width;
        viewport.MinDepth = MinDepth;
        viewport.MaxDepth = MaxDepth;
        return viewport;
    }

}
