#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <bitsery/bitsery.h>
#include <Utility/SerializationAdapters.hpp>

namespace PathFinder
{

    /**
     1 position
     1 normal
     1 texture coordinate
     */
    struct Vertex1P1N1UV 
    {
        glm::vec4 Position;
        glm::vec3 Normal;
        glm::vec2 UV;

        Vertex1P1N1UV() = default;

        Vertex1P1N1UV(
            const glm::vec4& position,
            const glm::vec3& normal,
            const glm::vec2& UV
        );

        template <typename S>
        void serialize(S& s)
        {
            s.object(Position);
            s.object(Normal);
            s.object(UV);
        }
    };

}
