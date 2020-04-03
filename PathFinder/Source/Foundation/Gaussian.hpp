#pragma once

#include <vector>

namespace Foundation
{

    namespace Gaussian
    {
        using Kernel = std::vector<float>;

        Kernel Kernel1D(size_t radius, float sigma);
        Kernel Kernel1D(size_t radius);
    };

}
