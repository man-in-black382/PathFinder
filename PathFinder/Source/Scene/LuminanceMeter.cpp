#include "LuminanceMeter.hpp"

#include <fplus/fplus.hpp>

namespace PathFinder 
{

    LuminanceMeter::LuminanceMeter(Camera* camera)
        : mCamera{ camera } {}
  
    void LuminanceMeter::SetHistogramData(const uint32_t* data)
    {
        mLuminanceBins.resize(mHistogramBinCount);
        std::copy(data, data + mHistogramBinCount, mLuminanceBins.begin());
        mMaxLuminanceBinSize = fplus::reduce([](uint32_t a, uint32_t b) -> uint32_t { return std::max(a, b); }, 0u, mLuminanceBins);
    }

}
