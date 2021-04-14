#include "Vertex1P1N1UV1T1BT.hpp"

namespace PathFinder
{

    Vertex1P1N1UV1T1BT::Vertex1P1N1UV1T1BT(
        const glm::vec4 &position,
        const glm::vec2 &texcoords,
        const glm::vec3 &normal,
        const glm::vec3 &tangent,
        const glm::vec3 &bitangent)
        :
        Vertex1P1N1UV(position, normal, texcoords),
        Tangent(tangent), Bitangent(bitangent) {}

    Vertex1P1N1UV1T1BT::Vertex1P1N1UV1T1BT(const glm::vec4 &position)
        : Vertex1P1N1UV(position, glm::vec3(0.0), glm::vec2(0.0)),
        Tangent(glm::vec3(0.0)), Bitangent(glm::vec3(0.0)) {}

    Vertex1P1N1UV1T1BT Vertex1P1N1UV1T1BT::TransformedBy(const Geometry::Transformation &t)
    {
        glm::mat4 modelMatrix = t.GetMatrix();
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(modelMatrix));

        return Vertex1P1N1UV1T1BT(
            modelMatrix * Position,
            UV,
            normalMatrix * glm::vec4(Normal, 1.0),
            normalMatrix * glm::vec4(Tangent, 1.0),
            normalMatrix * glm::vec4(Bitangent, 1.0)
        );
    }

}
