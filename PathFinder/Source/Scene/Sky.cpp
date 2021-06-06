#include "Sky.hpp"

#include <Foundation/Pi.hpp>
#include <hoseksky/ArHosekSkyModel.h>
#include <cmath>

namespace PathFinder
{

    Sky::Sky() 
        : mSkySpectrum{ 25, 400, 700 },
        mGroundAlbedoSpectrum{ 25, 400, 700 } 
    {
        mSkySpectrum.Init();
        mGroundAlbedoSpectrum.Init();
        mGroundAlbedoSpectrum.FromRGB(glm::vec3{ 0.1 });
    }

    void Sky::UpdateSolarIlluminance()
    {
        float thetaS = std::acos(1.0 - mSunDirection.y);
        float elevation = glm::clamp(M_PI_2 - thetaS, 0.001, 0.999);
        float turbidity = 1.7f;

        uint32_t totalSampleCount = mSkySpectrum.GetSamples().size();

        // Vertical sample angle. For one ray it's just equal to elevation.
        float theta = elevation;

        // Angle between the sun direction and sample direction.
        // Since we have only one sample for simplicity, the angle is 0.
        float gamma = 0.0; 

        // We compute spectrum for the middle ray at the center of the Sun's disk.
        // For simplicity, we ignore limb darkening.
        for (uint64_t i = 0; i < totalSampleCount; ++i)
        {
            ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(elevation, turbidity, mGroundAlbedoSpectrum[i]);
            float wavelength = glm::mix(mSkySpectrum.LowestWavelength(), mSkySpectrum.HighestWavelength(), i / float(totalSampleCount));
            mSkySpectrum[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));
            arhosekskymodelstate_free(skyState);
        }

        glm::vec3 sunLuminance = mSkySpectrum.ToRGB();
        glm::vec3 sunIlluminance = sunLuminance * SunSolidAngle; // Dividing by 1 / PDF

        //IC(sunIlluminance.r, sunIlluminance.g, sunIlluminance.b);

        // Why multiply by 100? Found it on internet, without it the illuminance is too small for some reason.
        mSolarIlluminance = sunIlluminance * 100.f; 
        mSolarLuminance = sunLuminance * 100.f;
    }

}
