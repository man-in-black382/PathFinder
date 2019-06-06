#pragma once

#include <glm/vec3.hpp>

namespace Geometry {

    struct Vertex1P3 {
        glm::vec3 position;

        Vertex1P3();

        Vertex1P3(glm::vec3 p);
    };

}
