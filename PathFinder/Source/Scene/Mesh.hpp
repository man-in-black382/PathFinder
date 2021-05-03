#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <optional>

#include "VertexStorageLocation.hpp"
#include "Vertices/Vertex1P1N1UV1T1BT.hpp"

#include <bitsery/bitsery.h>
#include <bitsery/traits/string.h>
#include <Geometry/AABB.hpp>

namespace PathFinder
{

    class Mesh
    {
    public:
        const std::string& GetName() const;
        std::vector<Vertex1P1N1UV1T1BT>& GetVertices();
        const std::vector<Vertex1P1N1UV1T1BT>& GetVertices() const;
        const std::vector<uint32_t>& GetIndices() const;
        const Geometry::AABB& GetBoundingBox() const;
        const VertexStorageLocation& GetLocationInVertexStorage() const;
        float GetSurfaceArea() const;
        bool HasTangentSpace() const;

        void SetName(const std::string& name);
        void SetHasTangentSpace(bool hts);
        void SetVertexStorageLocation(const VertexStorageLocation& location);
        void AddVertex(const Vertex1P1N1UV1T1BT& vertex);
        void AddIndex(uint32_t index);

        void SerializeVertexData(const std::filesystem::path& path);
        void DeserializeVertexData(const std::filesystem::path& path);

    private:
        friend bitsery::Access;

        template <typename S>
        void serialize(S& s)
        {
            s.container1b(mName, 1000);
            s.object(mBoundingBox);
            s.value4b(mArea);
            s.boolValue(mHasTangentSpace);
        }

        std::string mName;
        std::vector<Vertex1P1N1UV1T1BT> mVertices;
        std::vector<uint32_t> mIndices;
        VertexStorageLocation mVertexStorageLocation;
        Geometry::AABB mBoundingBox = Geometry::AABB::MaximumReversed();
        float mArea = 0.0;
        bool mHasTangentSpace = true;
    };

}
