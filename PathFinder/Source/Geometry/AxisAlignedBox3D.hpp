#pragma once

#include "Transformation.hpp"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <array>

namespace Geometry
{

    struct Triangle3D;

    struct AxisAlignedBox3D 
    {
        glm::vec3 Min = glm::zero<glm::vec3>();
        glm::vec3 Max = glm::zero<glm::vec3>();

        static const AxisAlignedBox3D &Zero();
        static const AxisAlignedBox3D &Unit();
        static const AxisAlignedBox3D &NDC();
        static const AxisAlignedBox3D &MaximumReversed();

        AxisAlignedBox3D() = default;
        AxisAlignedBox3D(const glm::vec3 &min, const glm::vec3 &max);

        float Diagonal() const;

        /**
         Represents box as an orthographic projection matrix
         as if it was directional light's frustum, for example

         @return orthographic projection matrix
         */
        glm::mat4 AsFrustum() const;
        glm::mat4 LocalSpaceMatrix() const;
        std::array<glm::vec4, 8> CornerPoints() const;
        float SmallestDimensionLength() const;
        float LargestDimensionLength() const;
        glm::vec3 Сenter() const;
        bool Contains(const glm::vec3 &point) const;
        bool Contains(const Triangle3D &triangle) const;
        bool Contains(const AxisAlignedBox3D &box) const;

        /**
         Splits into 8 sub-boxes of equal size
         The order is:
          Bottom-Front-Left
          Bottom-Front-Right
          Top-Front-Left
          Top-Front-Right
          Bottom-Back-Left
          Bottom-Back-Right
          Top-Back-Left
          Top-Back-Right

         @return array of sub-boxes
         */
        std::array<AxisAlignedBox3D, 8> Octet() const;
        AxisAlignedBox3D TransformedBy(const Transformation &t) const;
        AxisAlignedBox3D TransformedBy(const glm::mat4 &m) const;
        AxisAlignedBox3D Union(const AxisAlignedBox3D& otherBox);
    };

}
