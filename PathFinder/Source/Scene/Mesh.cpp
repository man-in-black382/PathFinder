#include "Mesh.hpp"

#include <Geometry/Triangle3D.hpp>
#include <bitsery/adapter/stream.h>
#include <bitsery/traits/vector.h>
#include <fstream>

namespace PathFinder
{

    const std::string& Mesh::Name() const
    {
        return mName;
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

    const Geometry::AABB& Mesh::BoundingBox() const
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

    void Mesh::SetVertexStorageLocation(const VertexStorageLocation& location)
    {
        mVertexStorageLocation = location;
    }

    void Mesh::AddVertex(const Vertex1P1N1UV1T1BT& vertex)
    {
        mBoundingBox.SetMin(glm::min(glm::vec3(vertex.Position), mBoundingBox.GetMin()));
        mBoundingBox.SetMax(glm::max(glm::vec3(vertex.Position), mBoundingBox.GetMax()));
        mVertices.push_back(vertex);

        if ((mVertices.size() % 3) == 0) {
            size_t count = mVertices.size();
            auto& v0 = mVertices[count - 3];
            auto& v1 = mVertices[count - 2];
            auto& v2 = mVertices[count - 1];

            Geometry::Triangle3D triangle(v0.Position, v1.Position, v2.Position);
            mArea += triangle.GetArea();
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

    void Mesh::SerializeVertexData(const std::filesystem::path& path)
    {
        std::fstream stream{ path, std::ios::binary | std::ios::trunc | std::ios::out };

        assert_format(stream.is_open(), "File (", path.string(), ") couldn't be opened for writing");

        bitsery::Serializer<bitsery::OutputBufferedStreamAdapter> ser{ stream };
        ser.container4b(mIndices, std::numeric_limits<uint64_t>::max());
        ser.container(mVertices, std::numeric_limits<uint64_t>::max());
        ser.adapter().flush();

        stream.close();
    }

    void Mesh::DeserializeVertexData(const std::filesystem::path& path)
    {
        std::fstream stream{ path, std::ios::binary | std::ios::in };

        assert_format(stream.is_open(), "File (", path.string(), ") couldn't be opened for reading");

        bitsery::Deserializer<bitsery::InputStreamAdapter> ser{ stream };
        ser.container4b(mIndices, std::numeric_limits<uint64_t>::max());
        ser.container(mVertices, std::numeric_limits<uint64_t>::max());

        stream.close();
    }

}


