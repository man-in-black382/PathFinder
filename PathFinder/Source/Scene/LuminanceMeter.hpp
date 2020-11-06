#pragma once

#include "Camera.hpp"

namespace PathFinder
{

    class LuminanceMeter
    {
    public:
        LuminanceMeter(Camera* camera);

        void SetHistogramData(const uint32_t* data);

    private:
        Camera* mCamera = nullptr;
        uint32_t mHistogramBinCount = 128;
        float mMinLumianance = 0.0;
        float mMaxLuminance = 1.0;
        std::vector<uint32_t> mHistogram;

    public:
        inline auto HistogramBinCount() const { return mHistogramBinCount; }
    };

}
