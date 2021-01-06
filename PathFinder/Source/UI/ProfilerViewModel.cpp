#include "ProfilerViewModel.hpp"

namespace PathFinder
{

    void ProfilerViewModel::Import()
    {
        if (mUpdateCooldown.Check())
        {
            mMeasurements = Dependencies->Device->Measurements();
        }
    }

}
