#pragma once

#include <vector>

namespace Foundation
{

    namespace GaussianFunction
    {
        using Kernel1D = std::vector<float>;

        Kernel1D Produce1DKernel(size_t radius, float sigma);
        Kernel1D Produce1DKernel(size_t radius);
    };

}
