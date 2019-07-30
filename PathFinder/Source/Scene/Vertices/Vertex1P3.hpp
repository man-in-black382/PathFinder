#pragma once

#include <glm/vec3.hpp>

namespace PathFinder 
{

    struct Vertex1P3 
    {
        Vertex1P3() = default;
        Vertex1P3(glm::vec3 p);

        glm::vec3 Position;
    };

}
