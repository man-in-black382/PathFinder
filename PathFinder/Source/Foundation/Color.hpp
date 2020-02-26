#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Foundation 
{

    struct Color 
    {
    public:
        enum class Space 
        {
            Linear, sRGB, YCoCg
        };

    public:
        static const Color &White();
        static const Color &Black();
        static const Color &Gray();
        static const Color &Red();
        static const Color &Blue();
        static const Color &Green();

        Color(float red, float green, float blue, float alpha, Space space = Space::Linear);
        Color(float red, float green, float blue, Space space = Space::Linear);
        Color(float white, float alpha, Space space = Space::Linear);
        Color(float white, Space space = Space::Linear);

        Color ConvertedTo(Space space) const;

        const float* Ptr() const { return &mR; }

    private:
        float mR = 0.0;
        float mG = 0.0;
        float mB = 0.0;
        float mA = 1.0;

        Space mSpace;

        Color ToLinear() const;

    public:
        inline float R() const { return mR; };
        inline float G() const { return mG; };
        inline float B() const { return mB; };
        inline float A() const { return mA; };

        inline Space CurrentSpace() const { return mSpace; }
    };

}
