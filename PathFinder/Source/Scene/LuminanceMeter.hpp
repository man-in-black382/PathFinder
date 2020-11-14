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
        uint32_t mHistogramBinCount = 128; // Must match constant in shader
        float mMinLumianance = 0.0;
        float mMaxLuminance = 1.0;
        uint32_t mMaxLuminanceBinSize = 0;
        std::vector<uint32_t> mLuminanceBins;

    public:
        inline auto HistogramBinCount() const { return mHistogramBinCount; }
        inline const auto& LuminanceBins() const { return mLuminanceBins; }
        inline auto MinLuminance() const { return mMinLumianance; }
        inline auto MaxLuminance() const { return mMaxLuminance; }
        inline auto MaxLuminanceBinSize() const { return mMaxLuminanceBinSize; }
    };

}
