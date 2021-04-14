#pragma once

#include "Transformation.hpp"

#include <Utility/SerializationAdapters.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <array>

namespace Geometry
{

    class AABB
    {
    public:
        static const AABB& Zero();
        static const AABB& Unit();
        static const AABB& NDC();
        static const AABB& MaximumReversed();

        template <class Iterator>
        AABB(Iterator pointsBegin, Iterator pointsEnd)
        {
            *this = MaximumReversed();

            while (pointsBegin != pointsEnd)
            {
                const glm::vec3& point = *pointsBegin;
                mMin = glm::min(mMin, point);
                mMax = glm::max(mMax, point);
                ++pointsBegin;
            }
        }

        AABB() = default;
        AABB(const glm::vec3& min, const glm::vec3& max);

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
        std::array<AABB, 8> Octet() const;
        AABB TransformedBy(const Transformation& t) const;
        AABB TransformedBy(const glm::mat4& m) const;
        AABB Union(const AABB& otherBox);

    private:
        friend bitsery::Access;

        template <typename S>
        void serialize(S& s)
        {
            s.object(mMin);
            s.object(mMax);
        }

        glm::vec3 mMin = glm::zero<glm::vec3>();
        glm::vec3 mMax = glm::zero<glm::vec3>();

    public:
        const glm::vec3& GetMin() const { return mMin; }
        void SetMin(const glm::vec3& min) { mMin = min; }
        const glm::vec3& GetMax() const { return mMax; }
        void SetMax(const glm::vec3& max) { mMax = max; }
    };

}
