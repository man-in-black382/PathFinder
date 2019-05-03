#pragma once

#include <glm/vec3.hpp>
#include <array>

namespace Geometry {

    struct AxisAlignedBox3D;

    template<typename Point>
    struct Triangle {
        union {
            struct {
                Point a;
                Point b;
                Point c;
            };

            struct {
                Point p1;
                Point p2;
                Point p3;
            };

            std::array<Point, 3> points;
            std::array<float, 3 * sizeof(Point) / sizeof(float)> values;
        };

        Triangle() = default;

        Triangle(const Point &p1, const Point &p2, const Point &p3) : a(p1), b(p2), c(p3) {
        }

        virtual float area() const = 0;
    };

}

