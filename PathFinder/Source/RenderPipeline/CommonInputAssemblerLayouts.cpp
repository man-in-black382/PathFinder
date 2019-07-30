#include "CommonInputAssemblerLayouts.hpp"

namespace PathFinder
{
    namespace CommonInputAssemblerLayouts
    {
     
        const HAL::InputAssemblerLayout& Layout1P1N1UV1T1BT()
        {
            static HAL::InputAssemblerLayout layout;
            return layout;
        }

        const HAL::InputAssemblerLayout& Layout1P1N1UV()
        {
            static HAL::InputAssemblerLayout layout;
            return layout;
        }

        const HAL::InputAssemblerLayout& Layout1P3()
        {
            static HAL::InputAssemblerLayout layout;
            layout.AddPerVertexLayoutElement("POSITION", 0, HAL::ResourceFormat::Color::RGBA32_Float, 0, 0);
            return layout;
        }

    }
}
