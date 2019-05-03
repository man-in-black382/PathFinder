#pragma once

#include <dxgi.h>

namespace HAL
{
    enum class TypedDataFormat {
        R8_Signed, RG8_Signed, RGBA8_Signed,
        R8_Unsigned, RG8_Unsigned, RGBA8_Unsigned,

        R16_Float, RG16_Float, RGBA16_Float,
        R16_Signed, RG16_Signed, RGBA16_Signed,
        R16_Unsigned, RG16_Unsigned, RGBA16_Unsigned,

        R32_Float, RG32_Float, RGB32_Float, RGBA32_Float,
        R32_Signed, RG32_Signed, RGB32_Signed, RGBA32_Signed,
        R32_Unsigned, RG32_Unsigned, RGB32_Unsigned, RGBA32_Unsigned
    };

    enum class TypelessDataFormat {
        R8, RG8, RGBA8,
        R16, RG16, RGBA16,
        R32, RG32, RGB32, RGBA32
    };

    namespace DataFormatResolving {

        constexpr DXGI_FORMAT DXGIFormat(TypedDataFormat format) {
            switch (format)
            {
            case TypedDataFormat::R8_Signed:         return DXGI_FORMAT_R8_SINT;
            case TypedDataFormat::RG8_Signed:        return DXGI_FORMAT_R8G8_SINT;
            case TypedDataFormat::RGBA8_Signed:      return DXGI_FORMAT_R8G8B8A8_SINT;

            case TypedDataFormat::R8_Unsigned:       return DXGI_FORMAT_R8_UINT;
            case TypedDataFormat::RG8_Unsigned:      return DXGI_FORMAT_R8G8_UINT;
            case TypedDataFormat::RGBA8_Unsigned:    return DXGI_FORMAT_R8G8B8A8_UINT;

            case TypedDataFormat::R16_Float:         return DXGI_FORMAT_R16_FLOAT;
            case TypedDataFormat::RG16_Float:        return DXGI_FORMAT_R16G16_FLOAT;
            case TypedDataFormat::RGBA16_Float:      return DXGI_FORMAT_R16G16B16A16_TYPELESS;

            case TypedDataFormat::R16_Signed:        return DXGI_FORMAT_R16_SINT;
            case TypedDataFormat::RG16_Signed:       return DXGI_FORMAT_R16G16_SINT;
            case TypedDataFormat::RGBA16_Signed:     return DXGI_FORMAT_R16G16B16A16_SINT;

            case TypedDataFormat::R16_Unsigned:      return DXGI_FORMAT_R16_UINT;
            case TypedDataFormat::RG16_Unsigned:     return DXGI_FORMAT_R16G16_UINT;
            case TypedDataFormat::RGBA16_Unsigned:   return DXGI_FORMAT_R16G16B16A16_UINT;

            case TypedDataFormat::R32_Float:         return DXGI_FORMAT_R32_FLOAT;
            case TypedDataFormat::RG32_Float:        return DXGI_FORMAT_R32G32_FLOAT;
            case TypedDataFormat::RGB32_Float:       return DXGI_FORMAT_R32G32B32_FLOAT;
            case TypedDataFormat::RGBA32_Float:      return DXGI_FORMAT_R32G32B32A32_FLOAT;

            case TypedDataFormat::R32_Signed:        return DXGI_FORMAT_R32_SINT;
            case TypedDataFormat::RG32_Signed:       return DXGI_FORMAT_R32G32_SINT;
            case TypedDataFormat::RGB32_Signed:      return DXGI_FORMAT_R32G32B32_SINT;
            case TypedDataFormat::RGBA32_Signed:     return DXGI_FORMAT_R32G32B32A32_SINT;

            case TypedDataFormat::R32_Unsigned:      return DXGI_FORMAT_R32_UINT;
            case TypedDataFormat::RG32_Unsigned:     return DXGI_FORMAT_R32G32_UINT;
            case TypedDataFormat::RGB32_Unsigned:    return DXGI_FORMAT_R32G32B32_UINT;
            case TypedDataFormat::RGBA32_Unsigned:   return DXGI_FORMAT_R32G32B32A32_UINT;
            }
        }

        constexpr DXGI_FORMAT DXGIFormat(TypelessDataFormat format) {
            switch (format)
            {
            case TypelessDataFormat::R8:                return DXGI_FORMAT_R8_TYPELESS;
            case TypelessDataFormat::RG8:               return DXGI_FORMAT_R8G8_TYPELESS;
            case TypelessDataFormat::RGBA8:             return DXGI_FORMAT_R8G8B8A8_TYPELESS;

            case TypelessDataFormat::R16:               return DXGI_FORMAT_R16_TYPELESS;
            case TypelessDataFormat::RG16:              return DXGI_FORMAT_R16G16_TYPELESS;
            case TypelessDataFormat::RGBA16:            return DXGI_FORMAT_R16G16B16A16_TYPELESS;

            case TypelessDataFormat::R32:               return DXGI_FORMAT_R32_TYPELESS;
            case TypelessDataFormat::RG32:              return DXGI_FORMAT_R32G32_TYPELESS;
            case TypelessDataFormat::RGB32:             return DXGI_FORMAT_R32G32B32_TYPELESS;
            case TypelessDataFormat::RGBA32:            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
            }
        }

    }
}

