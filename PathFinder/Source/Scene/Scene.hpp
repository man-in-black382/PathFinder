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
#include "LuminanceMeter.hpp"
#include "GIManager.hpp"
#include "SceneGPUStorage.hpp"

#include <Memory/GPUResourceProducer.hpp>
#include <robinhood/robin_hood.h>

#include <functional>
#include <vector>
#include <memory>
#include <filesystem>

namespace PathFinder 
{

    class Scene 
    {
    public:
        using FlatLightIt = std::list<FlatLight>::iterator;
        using SphericalLightIt = std::list<SphericalLight>::iterator;

        using LightVariant = std::variant<FlatLight*, SphericalLight*>;

        Scene(const std::filesystem::path& executableFolder, const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer);

        Mesh& AddMesh(Mesh&& mesh);
        MeshInstance& AddMeshInstance(MeshInstance&& instance);
        Material& AddMaterial(Material&& material);
        FlatLightIt EmplaceDiskLight();
        FlatLightIt EmplaceRectangularLight();
        SphericalLightIt EmplaceSphericalLight();

        void MapEntitiesToGPUIndices();

        void Serialize(const std::filesystem::path& destination) const;
        void Deserialize(const std::filesystem::path& source);

    private:
        void LoadUtilityResources();

        std::list<Mesh> mMeshes;
        std::list<MeshInstance> mMeshInstances;
        std::list<Material> mMaterials;
        std::list<FlatLight> mRectangularLights;
        std::list<FlatLight> mDiskLights;
        std::list<SphericalLight> mSphericalLights;

        Camera mCamera;
        LuminanceMeter mLuminanceMeter;
        GTTonemappingParameterss mTonemappingParams;
        BloomParameters mBloomParameters;
        GIManager mGIManager;

        Memory::GPUResourceProducer::TexturePtr mBlueNoiseTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAAAreaTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAASearchTexture;
        Mesh mUnitCube;
        Mesh mUnitSphere;

        ResourceLoader mResourceLoader;
        MeshLoader mMeshLoader;
        SceneGPUStorage mGPUStorage;

        std::vector<MeshInstance*> mMeshInstanceGPUIndexMappings;
        std::vector<LightVariant> mLightGPUIndexMappings;

    public:
        inline Camera& MainCamera() { return mCamera; }
        inline const Camera& MainCamera() const { return mCamera; }
        inline LuminanceMeter& LumMeter() { return mLuminanceMeter; }
        inline const LuminanceMeter& LumMeter() const { return mLuminanceMeter; }
        inline GIManager& GlobalIlluminationManager() { return mGIManager; }
        inline const GIManager& GlobalIlluminationManager() const { return mGIManager; }
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
        inline const Mesh& UnitSphere() const { return mUnitSphere; }

        inline MeshInstance* MeshInstanceForGPUIndex(uint64_t index) const { return mMeshInstanceGPUIndexMappings[index]; }
        inline LightVariant LightForGPUIndex(uint64_t index) const { return mLightGPUIndexMappings[index]; }

        inline SceneGPUStorage& GPUStorage() { return mGPUStorage; }
    };

}
