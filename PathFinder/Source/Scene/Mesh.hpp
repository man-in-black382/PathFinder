#pragma once

#include <vector>
#include <string>

#include "Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "../Geometry/AxisAlignedBox3D.hpp"
#include "../RenderPipeline/VertexStorageLocation.hpp"

namespace PathFinder
{

    class Mesh
    {
    public:
        const std::string& Name() const;
        const std::string& MaterialName() const;
        std::vector<Vertex1P1N1UV1T1BT>& Vertices();
        const std::vector<Vertex1P1N1UV1T1BT>& Vertices() const;
        const std::vector<uint32_t>& Indices() const;
        const Geometry::AxisAlignedBox3D& BoundingBox() const;
        const VertexStorageLocation& LocationInVertexStorage() const;
        float SurfaceArea() const;
        bool HasTangentSpace() const;

        void SetName(const std::string& name);
        void SetHasTangentSpace(bool hts);
        void SetMaterialName(const std::string& name);
        void SetVertexStorageLocation(const VertexStorageLocation& location);
        void AddVertex(const Vertex1P1N1UV1T1BT& vertex);
        void AddIndex(uint32_t index);

    private:
        std::string mName;
        std::string mMaterialName;
        std::vector<Vertex1P1N1UV1T1BT> mVertices;
        std::vector<uint32_t> mIndices;
        VertexStorageLocation mVertexStorageLocation;
        Geometry::AxisAlignedBox3D mBoundingBox = Geometry::AxisAlignedBox3D::MaximumReversed();
        float mArea = 0.0;
        bool mHasTangentSpace = true;
    };

}
