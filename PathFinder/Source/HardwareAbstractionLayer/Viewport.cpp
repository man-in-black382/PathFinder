#include "Viewport.hpp"

namespace HAL
{

    Viewport::Viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height, float minDepth, float maxDepth)
        : X(x), Y(y), Width(width), Height(height), MinDepth(minDepth), MaxDepth(maxDepth) {}

    Viewport::Viewport(uint32_t width, uint32_t height, float minDepth, float maxDepth)
        : Viewport(0, 0, width, height, minDepth, maxDepth) {}

    Viewport::Viewport(uint32_t width, uint32_t height)
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
