#pragma once

#include <Geometry/Transformation.hpp>
#include <Geometry/AABB.hpp>
#include <bitsery/bitsery.h>
#include <bitsery/ext/pointer.h>
#include <Utility/SerializationAdapters.hpp>

#include "Mesh.hpp"
#include "Material.hpp"

#include <unordered_map>
#include <glm/mat4x4.hpp>
#include <optional>
#include <cstdint>

namespace PathFinder
{

    class MeshInstance
    {
    public:
        MeshInstance(Mesh* mesh, Material* material);

        void UpdatePreviousFrameValues();

    private:
        friend bitsery::Access;

        MeshInstance() = default;

        template <typename S>
        void serialize(S& s)
        {
            s.ext(mMesh, bitsery::ext::PointerObserver{});
            s.ext(mMaterial, bitsery::ext::PointerObserver{});
            s.boolValue(mIsSelected);
            s.boolValue(mIsHighlighted);
            s.object(mPreviousTransformation);
            s.object(mTransformation);
        }

        Mesh* mMesh = nullptr;
        Material* mMaterial = nullptr;
        bool mIsSelected = false;
        bool mIsHighlighted = false;
        Geometry::Transformation mTransformation;
        Geometry::Transformation mPreviousTransformation;
        uint32_t mIndexInGPUTable = 0;

    public:
        inline bool IsSelected() const { return mIsSelected; }
        inline bool IsHighlighted() const { return mIsHighlighted; }
        inline const Geometry::Transformation& GetTransformation() const { return mTransformation; }
        inline const Geometry::Transformation& GetPreviousTransformation() const { return mPreviousTransformation; }
        inline Geometry::AABB GetBoundingBox(const Mesh& mesh) const { return mesh.GetBoundingBox().TransformedBy(mTransformation); }
        inline const Mesh* GetAssociatedMesh() const { return mMesh; }
        inline const Material* GetAssociatedMaterial() const { return mMaterial; }
        inline Mesh* GetAssociatedMesh() { return mMesh; }
        inline Material* GetAssociatedMaterial() { return mMaterial; }
        inline auto GetIndexInGPUTable () const { return mIndexInGPUTable; }

        inline void SetIsSelected(bool selected) { mIsSelected = selected; }
        inline void SetIsHighlighted(bool highlighted) { mIsHighlighted = highlighted; }
        inline void SetTransformation(const Geometry::Transformation& transform) { mTransformation = transform; }
        inline void SetIndexInGPUTable(uint32_t index) { mIndexInGPUTable = index; }
    };

}
