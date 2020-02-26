#include "Color.hpp"

#include <complex>

namespace Foundation
{

    const Color& Color::White()
    {
        static Color white(1.0f, 1.0f);
        return white;
    }

    const Color& Color::Black()
    {
        static Color black(0.0f, 1.0f);
        return black;
    }

    const Color& Color::Gray()
    {
        static Color gray(0.3f, 1.0f);
        return gray;
    }

    const Color& Color::Red()
    {
        static Color red(1.0f, 0.0f, 0.0f, 1.0f);
        return red;
    }

    const Color& Color::Blue()
    {
        static Color blue(0.0f, 0.0f, 1.0f, 1.0f);
        return blue;
    }

    const Color& Color::Green()
    {
        static Color green(0.0f, 1.0f, 0.0f, 1.0f);
        return green;
    }


    Color::Color(float red, float green, float blue, float alpha, Space space)
        : mR(red), mG(green), mB(blue), mA(alpha), mSpace(space) {}

    Color::Color(float red, float green, float blue, Space space) : Color(red, green, blue, 1.0, space) {}

    Color::Color(float white, float alpha, Space space) : Color(white, white, white, alpha, space) {}

    Color::Color(float white, Color::Space space) : Color(white, white, white, 1.0, space) {}

    Color Color::ToLinear() const
    {
        switch (mSpace) {
        case Color::Space::sRGB: 
        {
            float power = 2.2f;
            return { std::pow(mR, power), std::pow(mG, power), std::pow(mB, power), mA };
        }

        case Color::Space::YCoCg: 
        {
            float t = mR - mB;
            float g = mR + mB;
            float b = t - mG;
            float r = t + mG;
            return { r, g, b, mA };
        }

        case Color::Space::Linear: 
        {
            return *this;
        }

        default: return *this;
        }
    }

    Color Color::ConvertedTo(Color::Space space) const
    {
        Color linear = ToLinear();

        switch (space) {
        case Space::Linear: {
            return linear;
        }

        case Space::sRGB: {
            float power = 1.0f / 2.2f;
            return { std::pow(linear.mR, power), std::pow(linear.mG, power), std::pow(linear.mB, power), mA, Space::sRGB };
        }

        case Space::YCoCg: {
            float Co = (linear.mR - linear.mB) / 2.0f;
            float t = linear.mB + Co;
            float Cg = (linear.mG - t) / 2.0f;
            float Y = t + Cg;
            return { Y, Co, Cg, mA, Space::YCoCg };
        }

        default: return *this;
        }
    }

}
