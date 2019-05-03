#include <glm/vec2.hpp>

namespace Geometry {

    struct Size2D {
        float width = 0.f;
        float height = 0.f;

        static const Size2D &zero();

        static const Size2D &unit();

        Size2D() = default;

        Size2D(float w, float h);

        Size2D(float side);

        bool operator==(const Size2D &rhs);

        bool operator!=(const Size2D &rhs);

        Size2D transformedBy(const glm::vec2 &vector) const;

        Size2D makeUnion(const Size2D &size);
    };

}
