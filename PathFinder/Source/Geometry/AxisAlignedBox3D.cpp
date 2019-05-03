#include "Triangle3D.hpp"
#include "AxisAlignedBox3D.hpp"

#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec4.hpp>
#include <glm/gtx/transform.hpp>

namespace Geometry {

    const AxisAlignedBox3D &AxisAlignedBox3D::Zero() {
        static AxisAlignedBox3D b;
        return b;
    }

    const AxisAlignedBox3D &AxisAlignedBox3D::Unit() {
        static AxisAlignedBox3D b(glm::vec3(0.0), glm::vec3(1.0));
        return b;
    }

    const AxisAlignedBox3D &AxisAlignedBox3D::NDC() {
        static AxisAlignedBox3D b(glm::vec3(-1.0), glm::vec3(1.0));
        return b;
    }

    const AxisAlignedBox3D &AxisAlignedBox3D::MaximumReversed() {
        auto min = glm::vec3(std::numeric_limits<float>::max());
        auto max = glm::vec3(std::numeric_limits<float>::lowest());
        static AxisAlignedBox3D b(min, max);
        return b;
    }

    AxisAlignedBox3D::AxisAlignedBox3D(const glm::vec3 &min, const glm::vec3 &max)
            :
            min(min),
            max(max) {
    }

    float AxisAlignedBox3D::diagonal() const {
        return glm::length(max - min);
    }

    glm::mat4 AxisAlignedBox3D::asFrustum() const {
        // Z component shenanigans due to NDC and world handedness inconsistency
        return glm::ortho(min.x, max.x, min.y, max.y, -max.z, -min.z);
    }

    glm::mat4 AxisAlignedBox3D::localSpaceMatrix() const {
        glm::mat4 translation = glm::translate(-min);
        glm::vec3 bbAxisLengths = max - min;
        glm::mat4 scale = glm::scale(1.f / bbAxisLengths);
        return scale * translation;
    }

    std::array<glm::vec4, 8> AxisAlignedBox3D::cornerPoints() const {
        return std::array<glm::vec4, 8>{
                glm::vec4{min.x, min.y, min.z, 1.f},
                glm::vec4{min.x, max.y, min.z, 1.f},
                glm::vec4{max.x, max.y, min.z, 1.f},
                glm::vec4{max.x, min.y, min.z, 1.f},
                glm::vec4{min.x, min.y, max.z, 1.f},
                glm::vec4{min.x, max.y, max.z, 1.f},
                glm::vec4{max.x, max.y, max.z, 1.f},
                glm::vec4{max.x, min.y, max.z, 1.f}
        };
    }

    float AxisAlignedBox3D::smallestDimensionLength() const {
        float minXY = std::min(fabs(max.x - min.x), fabs(max.y - min.y));
        return std::min(fabs(max.z - min.z), minXY);
    }

    float AxisAlignedBox3D::largestDimensionLength() const {
        float maxXY = std::max(fabs(max.x - min.x), fabs(max.y - min.y));
        return std::max(fabs(max.z - min.z), maxXY);
    }

    glm::vec3 AxisAlignedBox3D::center() const {
        return (min + max) / 2.f;
    }

    bool AxisAlignedBox3D::contains(const glm::vec3 &point) const {
        return point.x >= min.x && point.x <= max.x &&
                point.y >= min.y && point.y <= max.y &&
                point.z >= min.z && point.z <= max.z;
    }

    bool AxisAlignedBox3D::contains(const Triangle3D &triangle) const {
        return contains(triangle.a) && contains(triangle.b) && contains(triangle.c);
    }

    bool AxisAlignedBox3D::contains(const AxisAlignedBox3D &box) const {
        return contains(box.min) && contains(box.max);
    }

    std::array<AxisAlignedBox3D, 8> AxisAlignedBox3D::octet() const {
        glm::vec3 c = center();
        return {
                AxisAlignedBox3D{min, c},
                AxisAlignedBox3D{{c.x, min.y, min.z}, {max.x, c.y, c.z}},
                AxisAlignedBox3D{{min.x, c.y, min.z}, {c.x, max.y, c.z}},
                AxisAlignedBox3D{{c.x, c.y, min.z}, {max.x, max.y, c.z}},
                AxisAlignedBox3D{{min.x, min.y, c.z}, {c.x, c.y, max.z}},
                AxisAlignedBox3D{{c.x, min.y, c.z}, {max.x, c.y, max.z}},
                AxisAlignedBox3D{{min.x, c.y, c.z}, {c.x, max.y, max.z}},
                AxisAlignedBox3D{c, max}
        };
    }

    AxisAlignedBox3D AxisAlignedBox3D::transformedBy(const Transformation &t) const {
        return transformedBy(t.modelMatrix());
    }

    AxisAlignedBox3D AxisAlignedBox3D::transformedBy(const glm::mat4 &m) const {
        glm::vec4 newMin = {min.x, min.y, min.z, 1.0};
        glm::vec4 newMax = {max.x, max.y, max.z, 1.0};
        newMin = m * newMin;
        newMax = m * newMax;
        newMin /= newMin.w;
        newMax /= newMax.w;
        return {glm::vec3(newMin), glm::vec3(newMax)};
    }

}
