#include "Sky.hpp"

#include <Foundation/Pi.hpp>
#include <Scene/Light.hpp>
#include <cmath>

namespace PathFinder
{

    Sky::Sky() 
        : mSkySpectrum{ 25, 400, 700 },
        mGroundAlbedoSpectrum{ 25, 400, 700 } 
    {
        mSkySpectrum.Init();
        mGroundAlbedoSpectrum.Init();
        mGroundAlbedoSpectrum.FromRGB(mGroundAlbedo);
    }

    void Sky::UpdateSkyState()
    {
        // Different sky model states seem to require elevation angles 
        // in different frames of reference, which is really confusing.
        float elevationPiOver2AtHorizon = std::acos(mSunDirection.y);
        float elevationPiOver2AtZenith = M_PI_2 - elevationPiOver2AtHorizon;
        float turbidity = 1.7f;

        uint32_t totalSampleCount = mSkySpectrum.GetSamples().size();

        // Vertical sample angle. For one ray it's just equal to elevation.
        float theta = elevationPiOver2AtHorizon;

        // Angle between the sun direction and sample direction.
        // Since we have only one sample for simplicity, the angle is 0.
        float gamma = 0.0; 

        // We compute spectrum for the middle ray at the center of the Sun's disk.
        // For simplicity, we ignore limb darkening.
        for (uint64_t i = 0; i < totalSampleCount; ++i)
        {
            ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(elevationPiOver2AtHorizon, turbidity, mGroundAlbedoSpectrum[i]);
            float wavelength = glm::mix(mSkySpectrum.LowestWavelength(), mSkySpectrum.HighestWavelength(), i / float(totalSampleCount));
            mSkySpectrum[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));
            arhosekskymodelstate_free(skyState);
        }

        if (mSkyModelStateR)
            arhosekskymodelstate_free(mSkyModelStateR);

        if (mSkyModelStateG)
            arhosekskymodelstate_free(mSkyModelStateG);

        if (mSkyModelStateB)
            arhosekskymodelstate_free(mSkyModelStateB);

        glm::vec3 sunLuminance = mSkySpectrum.ToRGB();
        glm::vec3 sunIlluminance = sunLuminance * SunSolidAngle; // Dividing by 1 / PDF

        // Found it on internet, without it the illuminance is too small.
        // Account for luminous efficacy, coordinate system scaling (100, wtf???)
        float multiplier = 100 * StandardLuminousEfficacy; 
        mSolarIlluminance = sunIlluminance * multiplier;
        mSolarLuminance = sunLuminance * multiplier;

        mSkyModelStateR = arhosek_rgb_skymodelstate_alloc_init(turbidity, mGroundAlbedo.r, elevationPiOver2AtZenith);
        mSkyModelStateG = arhosek_rgb_skymodelstate_alloc_init(turbidity, mGroundAlbedo.g, elevationPiOver2AtZenith);
        mSkyModelStateB = arhosek_rgb_skymodelstate_alloc_init(turbidity, mGroundAlbedo.b, elevationPiOver2AtZenith);
    }

    void Sky::UpdatePreviousFrameValues()
    {
        mPreviousSunDirection = mSunDirection;
    }

    void Sky::SetSunDirection(const glm::vec3& direction)
    {
        mSunDirection = direction;
        mSunDirection.y = glm::clamp(direction.y, 0.06f, 0.99f);
        mSunDirection = glm::normalize(mSunDirection);
    }

}
