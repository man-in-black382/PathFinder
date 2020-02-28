#pragma once

#include "Mesh.hpp"
#include "MeshInstance.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "GTTonemappingParameters.hpp"
#include "ResourceLoader.hpp"
#include "FlatLight.hpp"
#include "SphericalLight.hpp"

#include "../Memory/GPUResourceProducer.hpp"

#include <functional>
#include <list>
#include <memory>

namespace PathFinder 
{

    class Scene 
    {
    public:
        using FlatLightIt = std::list<FlatLight>::iterator;
        using SphericalLightIt = std::list<SphericalLight>::iterator;

        Mesh& AddMesh(Mesh&& mesh);
        MeshInstance& AddMeshInstance(MeshInstance&& instance);
        Material& AddMaterial(Material&& material);
        FlatLightIt EmplaceFlatLight(FlatLight::Type type);
        SphericalLightIt EmplaceSphericalLight();

        void IterateMeshInstances(const std::function<void(const MeshInstance& instance)>& functor) const;
        void IterateMeshInstances(const std::function<void(MeshInstance & instance)>& functor);

        void IterateMaterials(const std::function<void(const Material& material)>& functor) const;

    private:
        std::list<Mesh> mMeshes;
        std::list<MeshInstance> mMeshInstances;
        std::list<Material> mMaterials;
        std::list<FlatLight> mFlatLights;
        std::list<SphericalLight> mSphericalLights;

        Camera mCamera;
        GTTonemappingParams mTonemappingParams;

    public:
        inline Camera& MainCamera() { return mCamera; }
        inline const Camera& MainCamera() const { return mCamera; }
        inline const auto& Meshes() const { return mMeshes; }
        inline const auto& MeshInstances() const { return mMeshInstances; }
        inline auto& Meshes() { return mMeshes; }
        inline auto& MeshInstances() { return mMeshInstances; }
        inline auto& Materials() { return mMaterials; }
        inline auto& FlatLights() { return mFlatLights; }
        inline auto& SphericalLights() { return mSphericalLights; }
        inline auto& TonemappingParams() { return mTonemappingParams; }
        inline const auto& TonemappingParams() const { return mTonemappingParams; }
    };

}
