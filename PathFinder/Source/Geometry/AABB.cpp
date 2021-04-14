#include "Triangle3D.hpp"
#include "AABB.hpp"

#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace Geometry
{

    const AABB& AABB::Zero()
    {
        static AABB b;
        return b;
    }

    const AABB& AABB::Unit()
    {
        static AABB b(glm::vec3(0.0), glm::vec3(1.0));
        return b;
    }

    const AABB& AABB::NDC()
    {
        static AABB b(glm::vec3(-1.0), glm::vec3(1.0));
        return b;
    }

    const AABB& AABB::MaximumReversed()
    {
        auto min = glm::vec3(std::numeric_limits<float>::max());
        auto max = glm::vec3(std::numeric_limits<float>::lowest());
        static AABB b(min, max);
        return b;
    }

    AABB::AABB(const glm::vec3& min, const glm::vec3& max)
        : mMin(min), mMax(max) {}

    float AABB::Diagonal() const
    {
        return glm::length(mMax - mMin);
    }

    glm::mat4 AABB::AsFrustum() const
    {
        // Z component shenanigans due to NDC and world handedness inconsistency
        return glm::ortho(mMin.x, mMax.x, mMin.y, mMax.y, -mMax.z, -mMin.z);
    }

    glm::mat4 AABB::LocalSpaceMatrix() const
    {
        glm::mat4 translation = glm::translate(-mMin);
        glm::vec3 bbAxisLengths = mMax - mMin;
        glm::mat4 scale = glm::scale(1.f / bbAxisLengths);
        return scale * translation;
    }

    std::array<glm::vec4, 8> AABB::CornerPoints() const
    {
        return std::array<glm::vec4, 8> {
            glm::vec4{ mMin.x, mMin.y, mMin.z, 1.f },
                glm::vec4{ mMin.x, mMax.y, mMin.z, 1.f },
                glm::vec4{ mMax.x, mMax.y, mMin.z, 1.f },
                glm::vec4{ mMax.x, mMin.y, mMin.z, 1.f },
                glm::vec4{ mMin.x, mMin.y, mMax.z, 1.f },
                glm::vec4{ mMin.x, mMax.y, mMax.z, 1.f },
                glm::vec4{ mMax.x, mMax.y, mMax.z, 1.f },
                glm::vec4{ mMax.x, mMin.y, mMax.z, 1.f }
        };
    }

    float AABB::SmallestDimensionLength() const
    {
        float minXY = std::min(fabs(mMax.x - mMin.x), fabs(mMax.y - mMin.y));
        return std::min(fabs(mMax.z - mMin.z), minXY);
    }

    float AABB::LargestDimensionLength() const
    {
        float maxXY = std::max(fabs(mMax.x - mMin.x), fabs(mMax.y - mMin.y));
        return std::max(fabs(mMax.z - mMin.z), maxXY);
    }

    glm::vec3 AABB::Сenter() const
    {
        return (mMin + mMax) / 2.f;
    }

    std::array<AABB, 8> AABB::Octet() const
    {
        glm::vec3 c = Сenter();
        return {
                AABB{mMin, c},
                AABB{{c.x, mMin.y, mMin.z}, {mMax.x, c.y, c.z}},
                AABB{{mMin.x, c.y, mMin.z}, {c.x, mMax.y, c.z}},
                AABB{{c.x, c.y, mMin.z}, {mMax.x, mMax.y, c.z}},
                AABB{{mMin.x, mMin.y, c.z}, {c.x, c.y, mMax.z}},
                AABB{{c.x, mMin.y, c.z}, {mMax.x, c.y, mMax.z}},
                AABB{{mMin.x, c.y, c.z}, {c.x, mMax.y, mMax.z}},
                AABB{c, mMax}
        };
    }

    AABB AABB::TransformedBy(const Transformation& t) const
    {
        return TransformedBy(t.GetMatrix());
    }

    AABB AABB::TransformedBy(const glm::mat4& m) const
    {
        glm::vec4 newMin = { mMin.x, mMin.y, mMin.z, 1.0 };
        glm::vec4 newMax = { mMax.x, mMax.y, mMax.z, 1.0 };
        newMin = m * newMin;
        newMax = m * newMax;
        newMin /= newMin.w;
        newMax /= newMax.w;
        return { glm::vec3(newMin), glm::vec3(newMax) };
    }

    AABB AABB::Union(const AABB& otherBox)
    {
        return { glm::min(mMin, otherBox.mMin), glm::max(mMax, otherBox.mMax) };
    }

}
