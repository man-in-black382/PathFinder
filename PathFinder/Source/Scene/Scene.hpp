#pragma once

#include "Mesh.hpp"
#include "MeshInstance.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "GTTonemappingParameters.hpp"
#include "BloomParameters.hpp"
#include "ResourceLoader.hpp"
#include "MeshLoader.hpp"
#include "FlatLight.hpp"
#include "SphericalLight.hpp"

#include <Memory/GPUResourceProducer.hpp>

#include <functional>
#include <vector>
#include <memory>

#include <robinhood/robin_hood.h>

namespace PathFinder 
{

    class Scene 
    {
    public:
        using FlatLightIt = std::vector<FlatLight>::iterator;
        using SphericalLightIt = std::vector<SphericalLight>::iterator;

        using EntityVariant = std::variant<MeshInstance*, FlatLight*, SphericalLight*>;

        Scene(const std::filesystem::path& executableFolder, Memory::GPUResourceProducer* resourceProducer);

        Mesh& AddMesh(Mesh&& mesh);
        MeshInstance& AddMeshInstance(MeshInstance&& instance);
        Material& AddMaterial(Material&& material);
        FlatLightIt EmplaceDiskLight();
        FlatLightIt EmplaceRectangularLight();
        SphericalLightIt EmplaceSphericalLight();

        std::optional<EntityVariant> GetEntityByID(const EntityID& id) const;

        void RemapEntityIDs();

    private:
        void LoadUtilityResources();

        std::vector<Mesh> mMeshes;
        std::vector<MeshInstance> mMeshInstances;
        std::vector<Material> mMaterials;
        std::vector<FlatLight> mRectangularLights;
        std::vector<FlatLight> mDiskLights;
        std::vector<SphericalLight> mSphericalLights;

        robin_hood::unordered_flat_map<EntityID, EntityVariant> mMappedEntities;

        Camera mCamera;
        GTTonemappingParameterss mTonemappingParams;
        BloomParameters mBloomParameters;

        Memory::GPUResourceProducer::TexturePtr mBlueNoiseTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAAAreaTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAASearchTexture;
        Mesh mUnitCube;

        ResourceLoader mResourceLoader;
        MeshLoader mMeshLoader;

    public:
        inline Camera& MainCamera() { return mCamera; }
        inline const Camera& MainCamera() const { return mCamera; }
        inline const auto& Meshes() const { return mMeshes; }
        inline const auto& MeshInstances() const { return mMeshInstances; }
        inline const auto& Materials() const { return mMaterials; }
        inline const auto& RectangularLights() const { return mRectangularLights; }
        inline const auto& DiskLights() const { return mDiskLights; }
        inline const auto& SphericalLights() const { return mSphericalLights; }
        inline const auto& TonemappingParams() const { return mTonemappingParams; }
        inline const auto& BloomParams() const { return mBloomParameters; }

        inline auto& Meshes() { return mMeshes; }
        inline auto& MeshInstances() { return mMeshInstances; }
        inline auto& Materials() { return mMaterials; }
        inline auto& RectangularLights() { return mRectangularLights; }
        inline auto& DiskLights() { return mDiskLights; }
        inline auto& SphericalLights() { return mSphericalLights; }
        inline auto& TonemappingParams() { return mTonemappingParams; }
        inline auto& BloomParams() { return mBloomParameters; }

        inline auto TotalLightCount() const { return mRectangularLights.size() + mDiskLights.size() + mSphericalLights.size(); }

        inline const auto BlueNoiseTexture() const { return mBlueNoiseTexture.get(); }
        inline const auto SMAASearchTexture() const { return mSMAASearchTexture.get(); }
        inline const auto SMAAAreaTexture() const { return mSMAAAreaTexture.get(); }
        inline const Mesh& UnitCube() const { return mUnitCube; }
    };

}
