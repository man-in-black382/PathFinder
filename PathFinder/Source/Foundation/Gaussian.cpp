#include "Gaussian.hpp"

#include <cmath>

namespace Foundation 
{

    namespace Gaussian
    {
        float Gaussian(float x, float mu, float sigma)
        {
            // https://docs.opencv.org/2.4/modules/imgproc/doc/filtering.html#getgaussiankernel
            const double a = (x - mu) / sigma;
            return std::exp(-0.5 * a * a);
        }

        Kernel Kernel1D(size_t radius, float sigma)
        {
            float sum = 0;

            Kernel kernel(radius + 1, 0.0);

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

        Kernel Kernel1D(size_t radius)
        {
            return Kernel(radius, radius / 2.0);
        }
    }

}
