#pragma once

#include <cstdint>

namespace PathFinder
{

    struct RenderPassConfiguration
    {
        enum class DeviceType
        {
            Graphics, AsyncCompute
        };

        bool IsEnabled = true;
        DeviceType DeviceToBeInvokedOn = DeviceType::Graphics;
    };

}
