#include "MeshInstance.hpp"

namespace PathFinder
{

 /*   MeshInstance::MeshInstance(ID meshID, const Mesh& mesh) : mMeshID(meshID)
    {
        mTransformation = mesh.baseTransform();
        mModelMatrix = mTransformation.ModelMatrix();
    }*/

    bool MeshInstance::IsSelected() const
    {
        return mIsSelected;
    }

    bool MeshInstance::IsHighlighted() const 
    {
        return mIsHighlighted;
    }

    const glm::mat4 &MeshInstance::ModelMatrix() const 
    {
        return mModelMatrix;
    }

    const Geometry::Transformation &MeshInstance::Transformation() const
    {
        return mTransformation;
    }

    Geometry::Transformation& MeshInstance::Transformation()
    {
        return mTransformation;
    }

    Geometry::AxisAlignedBox3D MeshInstance::BoundingBox(const Mesh& mesh) const
    {
        return mesh.BoundingBox().TransformedBy(mTransformation);
    }

    /*std::optional<MaterialReference> MeshInstance::materialReferenceForSubMeshID(ID subMeshID) const
    {
        if (mSubMeshMaterialMap.find(subMeshID) == mSubMeshMaterialMap.end()) {
            return std::nullopt;
        }
        return mSubMeshMaterialMap.at(subMeshID);
    }*/


    void MeshInstance::SetIsSelected(bool selected)
    {
        mIsSelected = selected;
    }

    void MeshInstance::SetIsHighlighted(bool highlighted)
    {
        mIsHighlighted = highlighted;
    }

    void MeshInstance::SetTransformation(const Geometry::Transformation &transform)
    {
        mTransformation = transform;
        mModelMatrix = transform.ModelMatrix();
    }

  /*  void MeshInstance::setMaterialReferenceForSubMeshID(const MaterialReference &ref, ID subMeshID)
    {
        mSubMeshMaterialMap[subMeshID] = ref;
    }*/

}
