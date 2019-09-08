#pragma once

#include "../HardwareAbstractionLayer/InputAssemblerLayout.hpp"

namespace PathFinder
{
    enum class VertexLayout
    {
        Layout1P1N1UV1T1BT, Layout1P1N1UV, Layout1P3
    };

    const HAL::InputAssemblerLayout& InputAssemblerLayoutForVertexLayout(VertexLayout layout);
}
