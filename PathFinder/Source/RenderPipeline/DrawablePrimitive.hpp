#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <HardwareAbstractionLayer/PrimitiveTopology.hpp>

namespace PathFinder
{

    class DrawablePrimitive
    {
    public:
        inline static const std::array<glm::vec3, 4> UnitQuadVertices{ 
            glm::vec3{-0.5f, -0.5f, 0.0f}, glm::vec3{-0.5f, 0.5f, 0.0f}, glm::vec3{0.5f, 0.5f, 0.0f}, glm::vec3{0.5f, -0.5f, 0.0f} 
        };

        inline static const std::array<uint32_t, 6> UnitQuadIndices{ 0, 1, 3, 3, 1, 2 };

        inline static const DrawablePrimitive& Quad()
        {
            static DrawablePrimitive primitive{ 6, HAL::PrimitiveTopology::TriangleList };
            return primitive;
        }

        inline static const DrawablePrimitive& Triangle()
        {
            static DrawablePrimitive primitive{ 3, HAL::PrimitiveTopology::TriangleList };
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
