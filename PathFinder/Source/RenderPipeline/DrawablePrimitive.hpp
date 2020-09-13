#pragma once

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>

#include "../HardwareAbstractionLayer/PrimitiveTopology.hpp"

namespace PathFinder
{

    class DrawablePrimitive
    {
    public:
        inline static const DrawablePrimitive& Quad()
        {
            static DrawablePrimitive primitive{ 4, HAL::PrimitiveTopology::TriangleStrip };
            return primitive;
        }

        inline static const DrawablePrimitive& Triangle()
        {
            static DrawablePrimitive primitive{ 3, HAL::PrimitiveTopology::TriangleStrip };
            return primitive;
        }

        inline auto VertexCount() const { return mVertexCount; }
        inline auto Topology() const { return mTopology; }

    private:
        DrawablePrimitive(uint8_t vertexCount, HAL::PrimitiveTopology topology)
            : mVertexCount{ vertexCount }, mTopology{ topology } {}

        uint8_t mVertexCount;
        HAL::PrimitiveTopology mTopology;
    };

}
