#pragma once

#include <vector>
#include <string>

#include "../Geometry/AxisAlignedBox3D.hpp"

#include "SubMesh.hpp"

namespace PathFinder
{

    class Mesh
    {
    public:
        const std::string &Name() const;

        const Geometry::AxisAlignedBox3D &BoundingBox() const;

        const std::vector<SubMesh> &SubMeshes() const;

        std::vector<SubMesh> &SubMeshes();

        void SetName(const std::string &name);

        void AddSubMesh(const SubMesh& subMesh);

    private:
        std::string mName;
        Geometry::AxisAlignedBox3D mBoundingBox;
        std::vector<SubMesh> mSubMeshes;
    };

}
