#pragma once

#include "../Geometry/Transformation.hpp"
#include "../Geometry/AxisAlignedBox3D.hpp"

#include "Mesh.hpp"
#include "Material.hpp"

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
        uint32_t mGPUInstanceIndex = 0;

    public:
        inline bool IsSelected() const { return mIsSelected; }
        inline bool IsHighlighted() const { return mIsHighlighted; }
        inline const glm::mat4 &ModelMatrix() const { return mModelMatrix; }
        inline const Geometry::Transformation& Transformation() const { return mTransformation; }
        inline const Geometry::Transformation& PrevTransformation() { return mPrevTransformation; }
        inline Geometry::AxisAlignedBox3D BoundingBox(const Mesh& mesh) const { return mesh.BoundingBox().TransformedBy(mTransformation); }
        inline const Mesh* AssosiatedMesh() const { return mMesh; }
        inline const Material* AssosiatedMaterial() const { return mMaterial; }
        inline auto GPUInstanceIndex() const { return mGPUInstanceIndex; }

        inline void SetIsSelected(bool selected) { mIsSelected = selected; }
        inline void SetIsHighlighted(bool highlighted) { mIsHighlighted = highlighted; }
        inline void SetTransformation(const Geometry::Transformation& transform) { mTransformation = transform; }
        inline void SetGPUInstanceIndex(uint32_t index) { mGPUInstanceIndex = index; }
    };

}
