#pragma once

#include <string>
#include <optional>

namespace PathFinder
{

    struct GraphicsShaderFileNames
    {
        std::wstring VertexShaderFileName;
        std::wstring PixelShaderFileName;
        std::optional<std::wstring> GeometryShaderFileName = std::nullopt;
    };

    struct ComputeShaderFileNames
    {
        std::wstring ComputeShaderFileName;
    };

    struct RayTracingShaderFileNames
    {
        std::wstring RayGenShaderFileName;
        std::wstring ClosestHitShaderFileName;
        std::wstring MissShaderFileName;
        std::optional<std::wstring> AnyHitShaderFileName = std::nullopt;
        std::optional<std::wstring> IntersectionShaderFileName = std::nullopt;
    };

}
