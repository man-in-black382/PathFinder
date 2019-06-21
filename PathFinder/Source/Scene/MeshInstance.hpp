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
        struct GPUBufferLocation
        {
            uint64_t vertexBufferOffset;
            uint64_t vertexCount;
            uint64_t indexBufferOffset;
            uint64_t indexCount;
        };

        /// Material reference that overrides individual sub mesh materials if set
        //std::optional<MaterialReference> materialReference;

        MeshInstance(const Mesh* mesh, const std::vector<GPUBufferLocation>& subMeshGPUBufferLocations);

        bool IsSelected() const;

        bool IsHighlighted() const;

        const glm::mat4 &ModelMatrix() const;

        const Geometry::Transformation& Transformation() const;

        Geometry::Transformation& Transformation();

        Geometry::AxisAlignedBox3D BoundingBox(const Mesh& mesh) const;

        //std::optional<MaterialReference> materialReferenceForSubMeshID(ID subMeshID) const;

        void SetIsSelected(bool selected);

        void SetIsHighlighted(bool highlighted);

        void SetTransformation(const Geometry::Transformation &transform);

        //void setMaterialReferenceForSubMeshID(const MaterialReference &ref, ID subMeshID);

    private:
        bool mIsSelected = false;
        bool mIsHighlighted = false;
        Geometry::Transformation mTransformation;
        glm::mat4 mModelMatrix;
        std::vector<GPUBufferLocation> mSubMeshGPUDataLocations;
        //std::unordered_map<ID, MaterialReference> mSubMeshMaterialMap;
    };

}
