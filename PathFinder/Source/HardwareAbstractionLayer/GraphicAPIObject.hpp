#pragma once

#include <d3d12.h>
#include <string>

namespace HAL
{
    class GraphicAPIObject
    {
    public:
        void SetName(const std::string& name);

        GraphicAPIObject() = default;
        GraphicAPIObject(GraphicAPIObject&& that) = default;
        GraphicAPIObject& operator=(GraphicAPIObject&& that) = default;

        ~GraphicAPIObject() = default;

    protected:
        GraphicAPIObject(const GraphicAPIObject& that) = default;
        GraphicAPIObject& operator=(const GraphicAPIObject& that) = default;
    };
}
