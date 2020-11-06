#include "LuminanceMeterViewModel.hpp"

namespace PathFinder
{

    void LuminanceMeterViewModel::SetLuminanceMeter(LuminanceMeter* meter)
    {
        mLuminanceMeter = meter;
    }

    void LuminanceMeterViewModel::Import()
    {

        /*const Memory::Buffer* histogram = engine.ResourceStorage()->GetPerResourceData(PathFinder::ResourceNames::LuminanceHistogram)->Buffer.get();

        pickedGeometryInfo->Read<uint32_t>([&scene](const uint32_t* data)
            {
                if (data) scene.LumMeter().SetHistogramData(data);
            });*/
    }

    void LuminanceMeterViewModel::Export()
    {

    }

}
