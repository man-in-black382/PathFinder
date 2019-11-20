#pragma once

#include <cstdint>

namespace PathFinder
{

    class RenderPass;

    struct RenderPassConfiguration
    {
        enum class DeviceType
        {
            Graphics, AsyncCompute
        };

        bool IsEnabled = true;
        uint32_t InvocationsPerFrame = 1;
        DeviceType DeviceToBeInvokedOn = DeviceType::Graphics;
    };

}
