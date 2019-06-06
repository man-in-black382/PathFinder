#include "Mesh.hpp"
#include "MeshLoader.hpp"

namespace PathFinder
{

    const std::string &Mesh::Name() const
    {
        return mName;
    }

    const Geometry::AxisAlignedBox3D &Mesh::BoundingBox() const
    {
        return mBoundingBox;
    }

    const std::vector<SubMesh> &Mesh::SubMeshes() const 
    {
        return mSubMeshes;
    }

    std::vector<SubMesh> &Mesh::SubMeshes()
    {
        return mSubMeshes;
    }

    void Mesh::SetName(const std::string &name)
    {
        mName = name;
    }

    void Mesh::AddSubMesh(const SubMesh& subMesh)
    {
        mSubMeshes.push_back(subMesh);
        mBoundingBox = mBoundingBox.Union(subMesh.BoundingBox());
    }

}


