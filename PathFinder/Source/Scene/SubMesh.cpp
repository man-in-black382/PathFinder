#include "SubMesh.hpp"

#include "../Geometry/Triangle3D.hpp"

namespace PathFinder
{

    const std::string &SubMesh::Name() const
    {
        return mName;
    }

    const std::string &SubMesh::MaterialName() const
    {
        return mMaterialName;
    }

    const std::vector<Vertex1P1N1UV1T1BT> &SubMesh::Vertices() const
    {
        return mVertices;
    }

    const std::vector<uint32_t>& SubMesh::Indices() const
    {
        return mIndices;
    }

    const Geometry::AxisAlignedBox3D &SubMesh::BoundingBox() const
    {
        return mBoundingBox;
    }

    float SubMesh::SurfaceArea() const
    {
        return mArea;
    }

    void SubMesh::SetName(const std::string &name)
    {
        mName = name;
    }

    void SubMesh::SetMaterialName(const std::string &name)
    {
        mMaterialName = name;
    }

    void SubMesh::AddVertex(const Vertex1P1N1UV1T1BT &vertex)
    {
        mBoundingBox.Min = glm::min(glm::vec3(vertex.Position), mBoundingBox.Min);
        mBoundingBox.Max = glm::max(glm::vec3(vertex.Position), mBoundingBox.Max);
        mVertices.push_back(vertex);

        if ((mVertices.size() % 3) == 0) {
            size_t count = mVertices.size();
            auto &v0 = mVertices[count - 3];
            auto &v1 = mVertices[count - 2];
            auto &v2 = mVertices[count - 1];

            Geometry::Triangle3D triangle(v0.Position, v1.Position, v2.Position);
            mArea += triangle.area();
        }
    }

    void SubMesh::AddIndex(uint32_t index)
    {
        mIndices.push_back(index);
    }

}
