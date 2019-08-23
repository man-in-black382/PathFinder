#include "GaussianFunction.hpp"

#include <cmath>

namespace Foundation 
{

    namespace GaussianFunction
    {
        float Gaussian(float x, float mu, float sigma)
        {
            // https://docs.opencv.org/2.4/modules/imgproc/doc/filtering.html#getgaussiankernel
            const double a = (x - mu) / sigma;
            return std::exp(-0.5 * a * a);
        }

        Kernel1D Produce1DKernel(size_t radius, float sigma)
        {
            float sum = 0;

            Kernel1D kernel(radius + 1, 0.0);

            size_t kernelSize = 2 * radius + 1;
            size_t startIndex = kernelSize / 2;

            for (size_t i = startIndex; i < kernelSize; i++) {
                float weight = Gaussian(i, radius, sigma);
                kernel[i - startIndex] = weight;
                sum += weight;
            }

            sum *= 2.0;
            sum -= kernel[0];

            for (float &weight : kernel) {
                weight /= sum;
            }

            return kernel;
        }

        Kernel1D Produce1DKernel(size_t radius)
        {
            return Produce1DKernel(radius, radius / 2.0);
        }
    }

}
