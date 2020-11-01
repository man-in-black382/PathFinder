#pragma once

#include "Vertex1P1N1UV.hpp"

#include <Geometry/Transformation.hpp>
#include <bitsery/bitsery.h>
#include <bitsery/ext/inheritance.h>
#include <Utility/SerializationAdapters.hpp>

namespace PathFinder
{

    /**
     1 position
     1 normal
     2 uv channels
     1 tangent vector
     1 bitangent vector
     */
    struct Vertex1P1N1UV1T1BT : Vertex1P1N1UV 
    {
        glm::vec3 Tangent;
        glm::vec3 Bitangent;

        using Vertex1P1N1UV::Vertex1P1N1UV;

        Vertex1P1N1UV1T1BT() = default;

        Vertex1P1N1UV1T1BT(
            const glm::vec4& position,
            const glm::vec2& texcoords,
            const glm::vec3& normal,
            const glm::vec3& tangent,
            const glm::vec3& bitangent
        );

        Vertex1P1N1UV1T1BT(const glm::vec4 &position);

        Vertex1P1N1UV1T1BT TransformedBy(const Geometry::Transformation &t);

        template <typename S>
        void serialize(S& s)
        {
            s.ext(*this, bitsery::ext::BaseClass<Vertex1P1N1UV>{});
            s.object(Tangent);
            s.object(Bitangent);
        }
    };

}
