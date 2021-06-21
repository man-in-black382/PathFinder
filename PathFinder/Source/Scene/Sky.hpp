#pragma once

#include <glm/vec3.hpp>
#include <Foundation/Spectrum.hpp>
#include <hoseksky/ArHosekSkyModel.h>

namespace PathFinder 
{

    class Sky
    {
    public:
        inline static const float SunAngularRadius = 0.004712389; // Angular radius of the sun from Earth, in radians
        inline static const float SunSolidAngle = 0.00006807; // Average solid angle as seen from Earth
        inline static const float SunDiskRadius = 0.00471242378; // tan(SunAngularRadius). Disk is at a distance 1 from a surface.

        Sky();

        void UpdateSkyState();
        void UpdatePreviousFrameValues();

    private:
        glm::vec3 mGroundAlbedo = glm::vec3{ 0.5f };
        glm::vec3 mSolarIlluminance = glm::vec3{ 1.0f };
        glm::vec3 mSolarLuminance = glm::vec3{ 1.0f };
        glm::vec3 mSunDirection = glm::vec3{ 0.0, 1.0, 0.0 };
        glm::vec3 mPreviousSunDirection = glm::vec3{ 0.0, 1.0, 0.0 };
        Foundation::SampledSpectrum mSkySpectrum;
        Foundation::SampledSpectrum mGroundAlbedoSpectrum;
        ArHosekSkyModelState* mSkyModelStateR = nullptr;
        ArHosekSkyModelState* mSkyModelStateG = nullptr;
        ArHosekSkyModelState* mSkyModelStateB = nullptr;

    public:
        inline const glm::vec3& GetSolarIlluminance() const { return mSolarIlluminance; }
        inline const glm::vec3& GetSunDirection() const { return mSunDirection; }
        inline const glm::vec3& GetPreviousSunDirection() const { return mPreviousSunDirection; }
        inline const glm::vec3& GetSolarLuminance() const { return mSolarLuminance; }
        inline const ArHosekSkyModelState* GetSkyModelStateR() const { return mSkyModelStateR; }
        inline const ArHosekSkyModelState* GetSkyModelStateG() const { return mSkyModelStateG; }
        inline const ArHosekSkyModelState* GetSkyModelStateB() const { return mSkyModelStateB; }
        void SetSunDirection(const glm::vec3& direction);
    };

}
