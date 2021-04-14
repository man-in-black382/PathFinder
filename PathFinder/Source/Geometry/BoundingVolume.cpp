#include "BoundingVolume.hpp"

namespace Geometry
{

    void BoundingVolume::SetSphereCenter(const glm::vec3& center)
    {
        mMin = center;
    }

    void BoundingVolume::SetSphereRadius(float radius)
    {
        mMax[0] = radius;
    }

    void BoundingVolume::SetAABBMin(const glm::vec3& min)
    {
        mMin = min;
    }

    void BoundingVolume::SetAABBMax(const glm::vec3& max)
    {
        mMax = max;
    }

    void BoundingVolume::SetOOBBMin(const glm::vec3& min)
    {
        mMin = min;
    }

    void BoundingVolume::SetOOBBMax(const glm::vec3& max)
    {
        mMax = max;
    }

    const glm::vec3& BoundingVolume::GetSphereCenter() const
    {
        return mMin;
    }

    const glm::vec3& BoundingVolume::GetAABBMin() const
    {
        return mMin;
    }

    const glm::vec3& BoundingVolume::GetAABBMax() const
    {
        return mMax;
    }

    const glm::vec3& BoundingVolume::GetOOBBMin() const
    {
        return mMin;
    }

    const glm::vec3& BoundingVolume::GetOOBBMax() const
    {
        return mMax;
    }

    float BoundingVolume::GetSphereRadius() const
    {
        return mMax[0];
    }

    void BoundingVolume::ConvertToType(BVType value)
    {
        //if (value == mType)
        //    return;

        //switch (mType)
        //{
        //case BVType::Sphere:
        //{
        //    switch (value)
        //    {
        //    case BVType::AABB:
        //    {
        //        // Transform Sphere to AABBox
        //        glm::vec4 min;
        //        glm::vec4 max;

        //        ConvertSphereToAABBox(GetSphereCenter(), GetSphereRadius(), min, max);

        //        SetAABBoxMin(min);
        //        SetAABBoxMax(max);
        //    }
        //    break;
        //    case BVType::OOBB:
        //    {
        //        // Transform Sphere to OOBBox
        //        glm::vec4 min;
        //        glm::vec4 max;
        //        glm::mat4 invMatrix;
        //        glm::vec4 localCenter;

        //        G4::Invert(invMatrix, GetParentMatrix());

        //        // The center is local in position but not in rotation. (=> TransformVector)
        //        TransformVector(localCenter, invMatrix, GetSphereCenter());

        //        ConvertSphereToAABBox(localCenter, GetSphereRadius(), min, max);

        //        SetOOBBoxMin(min);
        //        SetOOBBoxMax(max);
        //    }
        //    break;
        //    default:
        //        assert_format(false, "Unknown BV type");
        //    }
        //}
        //break;
        //case BVType::AABB:
        //{
        //    switch (value)
        //    {
        //    case BVType::Sphere:
        //    {
        //        // Transform AABBox to sphere
        //        glm::vec4 center;
        //        float radius;

        //        ConvertAABBoxToSphere(GetAABBoxMin(), GetAABBoxMax(), center, radius);

        //        SetSphereCenter(center);
        //        SetSphereRadius(radius);
        //    }
        //    break;
        //    case BVType::OOBB:
        //    {
        //        // Transform AABBox to OOBBox
        //        glm::vec4 min;
        //        glm::vec4 max;

        //        ConvertAABBoxToOOBBox(GetParentMatrix(), GetAABBoxMin(), GetAABBoxMax(), min, max);

        //        SetOOBBoxMin(min);
        //        SetOOBBoxMax(max);
        //    }
        //    break;
        //    default:
        //        assert_format(false, "Unknown BV type");
        //    }
        //}
        //break;
        //case BVType::OOBB:
        //{
        //    switch (value)
        //    {
        //    case BVType::Sphere:
        //    {
        //        // Transform OOBBox to sphere
        //        glm::vec4 center;
        //        float radius;

        //        ConvertAABBoxToSphere(GetOOBBoxMin(), GetOOBBoxMax(), center, radius);

        //        SetSphereCenter(center);
        //        SetSphereRadius(radius);
        //    }
        //    break;
        //    case BVType::AABB:
        //    {
        //        // Transform OOBBox to AABBox
        //        glm::vec4  min;
        //        glm::vec4  max;

        //        ConvertOOBBoxToAABBox(GetParentMatrix(), GetOOBBoxMin(), GetOOBBoxMax(), min, max);

        //        SetAABBoxGlobalMin(min);
        //        SetAABBoxGlobalMax(max);
        //    }
        //    break;
        //    default:
        //        assert_format(false, "Unknown BV type");
        //    }
        //}
        //break;
        //default:
        //    assert_format(false, "Unknown BV type");
        //}
    }

}
