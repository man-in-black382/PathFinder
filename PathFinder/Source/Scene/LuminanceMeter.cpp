#include "LuminanceMeter.hpp"

namespace PathFinder 
{

    LuminanceMeter::LuminanceMeter(Camera* camera)
        : mCamera{ camera } {}
  
    void LuminanceMeter::SetHistogramData(const uint32_t* data)
    {
        mHistogram.resize(mHistogramBinCount);
        std::copy(data, data + mHistogramBinCount, mHistogram.begin());
        mMinLumianance = *((float*)(data + mHistogramBinCount));
        mMaxLuminance = *((float*)(data + mHistogramBinCount + 1));
    }

}
