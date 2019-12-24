#include "VertexLayouts.hpp"

#include <unordered_map>

namespace PathFinder
{

    const HAL::InputAssemblerLayout& InputAssemblerLayoutForVertexLayout(VertexLayout layout)
    {
        static std::unordered_map<VertexLayout, HAL::InputAssemblerLayout> layouts;

        if (layouts.find(VertexLayout::Layout1P3) == layouts.end())
        {
            HAL::InputAssemblerLayout& iaLayout = layouts[VertexLayout::Layout1P3];
            iaLayout.AddPerVertexLayoutElement("POSITION", 0, HAL::ResourceFormat::Color::RGBA32_Float, 0, 0);
        }

        if (layouts.find(VertexLayout::Layout1P1N1UV) == layouts.end()) 
        {
            HAL::InputAssemblerLayout& iaLayout = layouts[VertexLayout::Layout1P1N1UV];
            iaLayout.AddPerVertexLayoutElement("POSITION", 0, HAL::ResourceFormat::Color::RGBA32_Float, 0, 0);
        }

        if (layouts.find(VertexLayout::Layout1P1N1UV1T1BT) == layouts.end())
        {
            HAL::InputAssemblerLayout& iaLayout = layouts[VertexLayout::Layout1P1N1UV1T1BT];
            iaLayout.AddPerVertexLayoutElement("POSITION", 0, HAL::ResourceFormat::Color::RGBA32_Float, 0, 0);
            iaLayout.AddPerVertexLayoutElement("NORMAL", 0, HAL::ResourceFormat::Color::RGB32_Float, 0, 16);
            iaLayout.AddPerVertexLayoutElement("TEXCOORD", 0, HAL::ResourceFormat::Color::RG32_Float, 0, 28);  
            iaLayout.AddPerVertexLayoutElement("TANGENT", 0, HAL::ResourceFormat::Color::RGB32_Float, 0, 36);
            iaLayout.AddPerVertexLayoutElement("BITANGENT", 0, HAL::ResourceFormat::Color::RGB32_Float, 0, 48);
        }

   /*     static D3D12_INPUT_ELEMENT_DESC local_layout[] = {
           { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, IM_OFFSETOF(ImDrawVert, pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, IM_OFFSETOF(ImDrawVert, uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
           { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, IM_OFFSETOF(ImDrawVert, col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        };*/

        return layouts[layout];
    }

}
