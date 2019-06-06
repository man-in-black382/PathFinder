#include "Triangle3D.hpp"
#include "AxisAlignedBox3D.hpp"

#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace Geometry
{

    const AxisAlignedBox3D &AxisAlignedBox3D::Zero()
    {
        static AxisAlignedBox3D b;
        return b;
    }

    const AxisAlignedBox3D &AxisAlignedBox3D::Unit()
    {
        static AxisAlignedBox3D b(glm::vec3(0.0), glm::vec3(1.0));
        return b;
    }

    const AxisAlignedBox3D &AxisAlignedBox3D::NDC()
    {
        static AxisAlignedBox3D b(glm::vec3(-1.0), glm::vec3(1.0));
        return b;
    }

    const AxisAlignedBox3D &AxisAlignedBox3D::MaximumReversed()
    {
        auto min = glm::vec3(std::numeric_limits<float>::max());
        auto max = glm::vec3(std::numeric_limits<float>::lowest());
        static AxisAlignedBox3D b(min, max);
        return b;
    }

    AxisAlignedBox3D::AxisAlignedBox3D(const glm::vec3 &min, const glm::vec3 &max)
        : Min(min), Max(max) {}

    float AxisAlignedBox3D::Diagonal() const
    {
        return glm::length(Max - Min);
    }

    glm::mat4 AxisAlignedBox3D::AsFrustum() const
    {
        // Z component shenanigans due to NDC and world handedness inconsistency
        return glm::ortho(Min.x, Max.x, Min.y, Max.y, -Max.z, -Min.z);
    }

    glm::mat4 AxisAlignedBox3D::LocalSpaceMatrix() const
    {
        glm::mat4 translation = glm::translate(-Min);
        glm::vec3 bbAxisLengths = Max - Min;
        glm::mat4 scale = glm::scale(1.f / bbAxisLengths);
        return scale * translation;
    }

    std::array<glm::vec4, 8> AxisAlignedBox3D::CornerPoints() const
    {
        return std::array<glm::vec4, 8> {
            glm::vec4{ Min.x, Min.y, Min.z, 1.f },
                glm::vec4{ Min.x, Max.y, Min.z, 1.f },
                glm::vec4{ Max.x, Max.y, Min.z, 1.f },
                glm::vec4{ Max.x, Min.y, Min.z, 1.f },
                glm::vec4{ Min.x, Min.y, Max.z, 1.f },
                glm::vec4{ Min.x, Max.y, Max.z, 1.f },
                glm::vec4{ Max.x, Max.y, Max.z, 1.f },
                glm::vec4{ Max.x, Min.y, Max.z, 1.f }
        };
    }

    float AxisAlignedBox3D::SmallestDimensionLength() const
    {
        float minXY = std::min(fabs(Max.x - Min.x), fabs(Max.y - Min.y));
        return std::min(fabs(Max.z - Min.z), minXY);
    }

    float AxisAlignedBox3D::LargestDimensionLength() const
    {
        float maxXY = std::max(fabs(Max.x - Min.x), fabs(Max.y - Min.y));
        return std::max(fabs(Max.z - Min.z), maxXY);
    }

    glm::vec3 AxisAlignedBox3D::Сenter() const
    {
        return (Min + Max) / 2.f;
    }

    bool AxisAlignedBox3D::Contains(const glm::vec3 &point) const
    {
        return point.x >= Min.x && point.x <= Max.x &&
            point.y >= Min.y && point.y <= Max.y &&
            point.z >= Min.z && point.z <= Max.z;
    }

    bool AxisAlignedBox3D::Contains(const Triangle3D &triangle) const
    {
        return Contains(triangle.a) && Contains(triangle.b) && Contains(triangle.c);
    }

    bool AxisAlignedBox3D::Contains(const AxisAlignedBox3D &box) const
    {
        return Contains(box.Min) && Contains(box.Max);
    }

    std::array<AxisAlignedBox3D, 8> AxisAlignedBox3D::Octet() const
    {
        glm::vec3 c = Сenter();
        return {
                AxisAlignedBox3D{Min, c},
                AxisAlignedBox3D{{c.x, Min.y, Min.z}, {Max.x, c.y, c.z}},
                AxisAlignedBox3D{{Min.x, c.y, Min.z}, {c.x, Max.y, c.z}},
                AxisAlignedBox3D{{c.x, c.y, Min.z}, {Max.x, Max.y, c.z}},
                AxisAlignedBox3D{{Min.x, Min.y, c.z}, {c.x, c.y, Max.z}},
                AxisAlignedBox3D{{c.x, Min.y, c.z}, {Max.x, c.y, Max.z}},
                AxisAlignedBox3D{{Min.x, c.y, c.z}, {c.x, Max.y, Max.z}},
                AxisAlignedBox3D{c, Max}
        };
    }

    AxisAlignedBox3D AxisAlignedBox3D::TransformedBy(const Transformation &t) const
    {
        return TransformedBy(t.ModelMatrix());
    }

    AxisAlignedBox3D AxisAlignedBox3D::TransformedBy(const glm::mat4 &m) const
    {
        glm::vec4 newMin = { Min.x, Min.y, Min.z, 1.0 };
        glm::vec4 newMax = { Max.x, Max.y, Max.z, 1.0 };
        newMin = m * newMin;
        newMax = m * newMax;
        newMin /= newMin.w;
        newMax /= newMax.w;
        return { glm::vec3(newMin), glm::vec3(newMax) };
    }

    AxisAlignedBox3D AxisAlignedBox3D::Union(const AxisAlignedBox3D& otherBox)
    {
        return { glm::min(Min, otherBox.Min), glm::max(Max, otherBox.Max) };
    }

}
