#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace Geometry
{

    class BoundingVolume 
    {
    public:
        enum class BVType
        {
            AABB, OOBB, Sphere
        };

        void SetSphereCenter(const glm::vec3& center);
        void SetSphereRadius(float value);
        void SetAABBMin(const glm::vec3& min);
        void SetAABBMax(const glm::vec3& max);
        void SetOOBBMin(const glm::vec3& min);
        void SetOOBBMax(const glm::vec3& max);

        float GetSphereRadius() const;
        const glm::vec3& GetSphereCenter() const;
        const glm::vec3& GetAABBMin() const;
        const glm::vec3& GetAABBMax() const;
        const glm::vec3& GetOOBBMin() const;
        const glm::vec3& GetOOBBMax() const;

        void ConvertToType(BVType value);

    private:
        glm::vec3 mMin;
        glm::vec3 mMax;
        BVType mType;
    };

}
