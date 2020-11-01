#pragma once

#include <vector>
#include <string>

#include "VertexStorageLocation.hpp"
#include "Vertices/Vertex1P1N1UV1T1BT.hpp"

#include <bitsery/bitsery.h>
#include <Geometry/AxisAlignedBox3D.hpp>


namespace PathFinder
{

    class Mesh
    {
    public:
        const std::string& Name() const;
        std::vector<Vertex1P1N1UV1T1BT>& Vertices();
        const std::vector<Vertex1P1N1UV1T1BT>& Vertices() const;
        const std::vector<uint32_t>& Indices() const;
        const Geometry::AxisAlignedBox3D& BoundingBox() const;
        const VertexStorageLocation& LocationInVertexStorage() const;
        float SurfaceArea() const;
        bool HasTangentSpace() const;

        void SetName(const std::string& name);
        void SetHasTangentSpace(bool hts);
        void SetVertexStorageLocation(const VertexStorageLocation& location);
        void AddVertex(const Vertex1P1N1UV1T1BT& vertex);
        void AddIndex(uint32_t index);

    private:
        friend bitsery::Access;

        template <typename S>
        void serialize(S& s)
        {
            s.text(mName);
            s.container(mVertices);
            s.container(mIndices);
            s.object(mBoundingBox.Min);
            s.object(mBoundingBox.Max);
            s.value(mArea);
            s.value(mHasTangentSpace);
        }

        std::string mName;
        std::vector<Vertex1P1N1UV1T1BT> mVertices;
        std::vector<uint32_t> mIndices;
        VertexStorageLocation mVertexStorageLocation;
        Geometry::AxisAlignedBox3D mBoundingBox = Geometry::AxisAlignedBox3D::MaximumReversed();
        float mArea = 0.0;
        bool mHasTangentSpace = true;
    };

}
