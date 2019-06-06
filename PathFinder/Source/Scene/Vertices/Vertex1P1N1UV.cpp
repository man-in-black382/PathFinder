#include "Vertex1P1N1UV.hpp"

namespace PathFinder 
{

    Vertex1P1N1UV::Vertex1P1N1UV(const glm::vec4& position, const glm::vec3& normal, const glm::vec2& texCoords)
        : Position(position), Normal(normal), UV(texCoords) {}

}
