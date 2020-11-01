#pragma once

#include <dxgi1_6.h>
#include <wrl.h>
#include <glm/vec2.hpp>
#include <Geometry/Rect2D.hpp>

#include "ResourceFormat.hpp"

namespace HAL
{
    class Display
    {
    public:
        Display(const DXGI_OUTPUT_DESC1& output);
    
    private:
        ColorSpace mColorSpace = ColorSpace::Rec709;
        glm::vec2 mRedPrimary;
        glm::vec2 mGreenPrimary;
        glm::vec2 mBluePrimary;
        glm::vec2 mWhitePoint;
        float mMinLuminance = 0.0f;
        float mMaxLuminance = 1.0f;
        float mMaxFullFrameLuminance = 1.0f;
        bool mIsHDRSupported = false;
        Geometry::Rect2D mDesktopRect;

    public:
        inline const auto& DisplayColorSpace() const { return mColorSpace; }
        inline const auto& RedPrimary() const { return mRedPrimary; }
        inline const auto& reenPrimary() const { return mGreenPrimary; }
        inline const auto& BluePrimary() const { return mBluePrimary; }
        inline const auto& WhitePoint() const { return mWhitePoint; }
        inline const auto& MinLuminance() const { return mMinLuminance; }
        inline const auto& MaxLuminance() const { return mMaxLuminance; }
        inline const auto& MaxFullFrameLuminance() const { return mMaxFullFrameLuminance; }
        inline const auto& IsHDRSupported() const { return mIsHDRSupported; }
        inline const auto& DesktopRect() const { return mDesktopRect; }
    };
}

