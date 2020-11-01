#include "Display.hpp"

namespace HAL
{

    Display::Display(const DXGI_OUTPUT_DESC1& output)
    {
        mColorSpace = ColorSpaceFromD3DSpace(output.ColorSpace);
        mRedPrimary = { output.RedPrimary[0], output.RedPrimary[1] };
        mGreenPrimary = { output.GreenPrimary[0], output.GreenPrimary[1] };
        mBluePrimary = { output.BluePrimary[0], output.BluePrimary[1] };
        mWhitePoint = { output.WhitePoint[0], output.WhitePoint[1] };
        mMinLuminance = output.MinLuminance;
        mMaxLuminance = output.MaxLuminance;
        mMaxFullFrameLuminance = output.MaxFullFrameLuminance;
        mIsHDRSupported = mColorSpace == ColorSpace::Rec2020;
        mDesktopRect = Geometry::Rect2D{
            glm::vec2(output.DesktopCoordinates.left, output.DesktopCoordinates.top),
            Geometry::Size2D(
                output.DesktopCoordinates.right - output.DesktopCoordinates.left, 
                output.DesktopCoordinates.bottom - output.DesktopCoordinates.top
            )
        };
    }

}
