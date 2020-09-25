#include "Mesh.hpp"

#include "../Geometry/Triangle3D.hpp"

namespace PathFinder
{

    const std::string& Mesh::Name() const
    {
        return mName;
    }

    const std::string& Mesh::MaterialName() const
    {
        return mMaterialName;
    }

    std::vector<Vertex1P1N1UV1T1BT>& Mesh::Vertices()
    {
        return mVertices;
    }

    const std::vector<Vertex1P1N1UV1T1BT>& Mesh::Vertices() const
    {
        return mVertices;
    }

    const std::vector<uint32_t>& Mesh::Indices() const
    {
        return mIndices;
    }

    const Geometry::AxisAlignedBox3D& Mesh::BoundingBox() const
    {
        return mBoundingBox;
    }

    const VertexStorageLocation& Mesh::LocationInVertexStorage() const
    {
        return mVertexStorageLocation;
    }

    float Mesh::SurfaceArea() const
    {
        return mArea;
    }

    bool Mesh::HasTangentSpace() const
    {
        return mHasTangentSpace;
    }

    void Mesh::SetName(const std::string& name)
    {
        mName = name;
    }

    void Mesh::SetHasTangentSpace(bool hts)
    {
        mHasTangentSpace = hts;
    }

    void Mesh::SetMaterialName(const std::string& name)
    {
        mMaterialName = name;
    }

    void Mesh::SetVertexStorageLocation(const VertexStorageLocation& location)
    {
        mVertexStorageLocation = location;
    }

    void Mesh::AddVertex(const Vertex1P1N1UV1T1BT& vertex)
    {
        mBoundingBox.Min = glm::min(glm::vec3(vertex.Position), mBoundingBox.Min);
        mBoundingBox.Max = glm::max(glm::vec3(vertex.Position), mBoundingBox.Max);
        mVertices.push_back(vertex);

        if ((mVertices.size() % 3) == 0) {
            size_t count = mVertices.size();
            auto& v0 = mVertices[count - 3];
            auto& v1 = mVertices[count - 2];
            auto& v2 = mVertices[count - 1];

            Geometry::Triangle3D triangle(v0.Position, v1.Position, v2.Position);
            mArea += triangle.area();
        }

        if (glm::length2(vertex.Tangent) <= 0.0 || glm::length2(vertex.Bitangent) <= 0)
        {
            mHasTangentSpace = false;
        }
    }

    void Mesh::AddIndex(uint32_t index)
    {
        mIndices.push_back(index);
    }

}


