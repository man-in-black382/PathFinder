#include <glm/vec2.hpp>

namespace Geometry 
{

    struct Size2D 
    {
        float Width = 0.f;
        float Height = 0.f;

        static const Size2D &Zero();
        static const Size2D &Unit();

        Size2D() = default;
        Size2D(float w, float h);
        Size2D(float side);

        bool operator==(const Size2D &rhs);
        bool operator!=(const Size2D &rhs);

        Size2D TransformedBy(const glm::vec2 &vector) const;
        Size2D MakeUnion(const Size2D &size);
    };

}
