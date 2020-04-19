#pragma once

#include "../Foundation/Color.hpp"

namespace PathFinder 
{
    using Lumen = float;
    using Nit = float;
    using Kelvin = float;
    using Candela = float;

    class Light
    {
    public:
        virtual ~Light() = 0;

        void SetGPULightTableIndex(uint32_t index);
        void SetColor(const Foundation::Color& color);
        void SetColorTemperature(Kelvin temperature);

        /// Sets Luminous Power a.k.a Luminous Flux.
        /// Measure of the perceived power of light.
        void SetLuminousPower(Lumen luminousPower);

    protected:
        void SetArea(float area);

    private:
        Lumen mLuminousPower = 0.0;
        Nit mLuminance = 0.0;
        Foundation::Color mColor = Foundation::Color::White();
        float mArea = 0.0;
        uint32_t mGPULightTableIndex;

    public:
        Lumen LuminousPower() const { return mLuminousPower; }
        Nit Luminance() const { return mLuminance; }
        auto GPULightTableIndex() const { return mGPULightTableIndex; }
        const Foundation::Color& Color() const { return mColor; }
    };

}
