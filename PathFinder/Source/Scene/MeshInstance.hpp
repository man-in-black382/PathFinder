#pragma once

#include "../Geometry/Transformation.hpp"
#include "../Geometry/AxisAlignedBox3D.hpp"

#include "Mesh.hpp"

#include <unordered_map>
#include <glm/mat4x4.hpp>
#include <optional>
#include <cstdint>

namespace PathFinder
{

    class MeshInstance
    {
    public:
        

        /// Material reference that overrides individual sub mesh materials if set
        //std::optional<MaterialReference> materialReference;

        MeshInstance(const Mesh* mesh);

        //void setMaterialReferenceForSubMeshID(const MaterialReference &ref, ID subMeshID);

    private:
        const Mesh* mMesh;
        bool mIsSelected = false;
        bool mIsHighlighted = false;
        Geometry::Transformation mTransformation;
        glm::mat4 mModelMatrix;
        //std::unordered_map<ID, MaterialReference> mSubMeshMaterialMap;

    public:
        inline bool IsSelected() const { return mIsSelected; }
        inline bool IsHighlighted() const { return mIsHighlighted; }
        inline const glm::mat4 &ModelMatrix() const { return mModelMatrix; }
        inline const Geometry::Transformation& Transformation() const { return mTransformation; }
        inline Geometry::Transformation& Transformation() { return mTransformation; }
        inline Geometry::AxisAlignedBox3D BoundingBox(const Mesh& mesh) const { return mesh.BoundingBox().TransformedBy(mTransformation); }
        inline const Mesh* AssosiatedMesh() const { return mMesh; }

        inline void SetIsSelected(bool selected) { mIsSelected = selected; }
        inline void SetIsHighlighted(bool highlighted) { mIsHighlighted = highlighted; }
        inline void SetTransformation(const Geometry::Transformation &transform) { mTransformation = transform; }
    };

}
