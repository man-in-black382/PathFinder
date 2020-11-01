#include "Light.hpp"

#include <Foundation/Pi.hpp>

namespace PathFinder
{

    Light::~Light() {}

    void Light::SetColor(const Foundation::Color& color)
    {
        mColor = color;
    }

    void Light::SetColorTemperature(Kelvin temperature)
    {
        // Convert to color here
    }

    void Light::SetLuminousPower(Lumen luminousPower)
    {
        mLuminousPower = luminousPower;
        mLuminance = mLuminousPower / mArea / M_PI;

        // Luminance due to a point on a Lambertian emitter, emitted in any direction, 
        // is equal to its total luminous power Phi divided by the emitter area A and the projected solid angle (Pi)
    }

    void Light::SetEntityID(EntityID id)
    {
        mEntityID = id;
    }

    void Light::SetVertexStorageLocation(const VertexStorageLocation& location)
    {
        mVertexStorageLocation = location;
    }

    void Light::SetIndexInGPUTable(uint32_t index)
    {
        mIndexInGPUTable = index;
    }

    void Light::SetArea(float area)
    {
        mArea = area;
        // Recalculate due to changes in the area
        SetLuminousPower(mLuminousPower);
    }

}
