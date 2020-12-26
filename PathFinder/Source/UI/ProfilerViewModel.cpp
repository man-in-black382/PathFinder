#include "ProfilerViewModel.hpp"

namespace PathFinder
{

    void ProfilerViewModel::Import()
    {
        if (mUpdateCooldown.CheckThenUpdate())
        {
            mMeasurements = Dependencies->Device->Measurements();
        }
    }

}
