#pragma once

#include <Geometry/Transformation.hpp>
#include <Geometry/AxisAlignedBox3D.hpp>

#include "Mesh.hpp"
#include "Material.hpp"
#include "EntityID.hpp"

#include <unordered_map>
#include <glm/mat4x4.hpp>
#include <optional>
#include <cstdint>

namespace PathFinder
{

    class MeshInstance
    {
    public:
        MeshInstance(const Mesh* mesh, const Material* material);

        void UpdatePreviousTransform();

    private:
        const Mesh* mMesh;
        const Material* mMaterial;
        bool mIsSelected = false;
        bool mIsHighlighted = false;
        Geometry::Transformation mTransformation;
        Geometry::Transformation mPrevTransformation;
        glm::mat4 mModelMatrix;
        EntityID mEntityID = 0;
        uint32_t mIndexInGPUTable = 0;

    public:
        inline bool IsSelected() const { return mIsSelected; }
        inline bool IsHighlighted() const { return mIsHighlighted; }
        inline const Geometry::Transformation& Transformation() const { return mTransformation; }
        inline const Geometry::Transformation& PrevTransformation() { return mPrevTransformation; }
        inline Geometry::AxisAlignedBox3D BoundingBox(const Mesh& mesh) const { return mesh.BoundingBox().TransformedBy(mTransformation); }
        inline const Mesh* AssociatedMesh() const { return mMesh; }
        inline const Material* AssociatedMaterial() const { return mMaterial; }
        inline const EntityID& ID() const { return mEntityID; }
        inline auto IndexInGPUTable () const { return mIndexInGPUTable; }

        inline void SetIsSelected(bool selected) { mIsSelected = selected; }
        inline void SetIsHighlighted(bool highlighted) { mIsHighlighted = highlighted; }
        inline void SetTransformation(const Geometry::Transformation& transform) { mTransformation = transform; }
        inline void SetIndexInGPUTable(uint32_t index) { mIndexInGPUTable = index; }
        inline void SetEntityID(EntityID id) { mEntityID = id; }
    };

}
