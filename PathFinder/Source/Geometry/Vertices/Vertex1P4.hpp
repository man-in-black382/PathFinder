#pragma once

#include <glm/vec4.hpp>

namespace Geometry {

    struct Vertex1P4 {
        glm::vec4 position;

        Vertex1P4();

        Vertex1P4(glm::vec4 p);
    };

}

