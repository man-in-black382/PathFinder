#pragma once

#include "Vertex1P1N2UV.hpp"
#include "../Transformation.hpp"

namespace Geometry {

    /**
     1 position
     1 normal
     2 uv channels
     1 tangent vector
     1 bitangent vector
     */
    struct Vertex1P1N2UV1T1BT : Vertex1P1N2UV {
        glm::vec3 tangent;
        glm::vec3 bitangent;

        using Vertex1P1N2UV::Vertex1P1N2UV;

        Vertex1P1N2UV1T1BT(const glm::vec4 &position,
                const glm::vec3 &texcoords,
                const glm::vec2 &lightmapCoords,
                const glm::vec3 &normal,
                const glm::vec3 &tangent,
                const glm::vec3 &bitangent);

        Vertex1P1N2UV1T1BT(const glm::vec4 &position);

        Vertex1P1N2UV1T1BT transformedBy(const Transformation &t);
    };

}
