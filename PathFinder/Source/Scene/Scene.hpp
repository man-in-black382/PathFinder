#pragma once

#include "Mesh.hpp"
#include "MeshInstance.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "GTTonemappingParameters.hpp"
#include "BloomParameters.hpp"
#include "ResourceLoader.hpp"
#include "FlatLight.hpp"
#include "SphericalLight.hpp"
#include "LuminanceMeter.hpp"
#include "GIManager.hpp"
#include "SceneGPUStorage.hpp"
#include "ThirdPartySceneLoader.hpp"
#include "MaterialLoader.hpp"
#include "Sky.hpp"

#include <Memory/GPUResourceProducer.hpp>
#include <RenderPipeline/PipelineResourceStorage.hpp>
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

        Scene(
            const std::filesystem::path& executableFolder, 
            const HAL::Device* device,
            Memory::GPUResourceProducer* resourceProducer,
            const PipelineResourceStorage* pipelineResourceStorage,
            const RenderSurfaceDescription* renderSurfaceDescription,
            const RenderSettings* renderSettings
        );

        Mesh& AddMesh(Mesh&& mesh);
        MeshInstance& AddMeshInstance(MeshInstance&& instance);
        Material& AddMaterial(Material&& material);
        FlatLightIt EmplaceDiskLight();
        FlatLightIt EmplaceRectangularLight();
        SphericalLightIt EmplaceSphericalLight();

        void MapEntitiesToGPUIndices();
        void UpdatePreviousFrameValues();

        void LoadThirdPartyScene(const std::filesystem::path& path, const ThirdPartySceneLoader::Settings& settings = {});
        void Serialize(const std::filesystem::path& destination);
        void Deserialize(const std::filesystem::path& source);

    private:
        struct FileStructure
        {
            FileStructure(const std::filesystem::path& sceneFilePath);

            void CreateDirectories() const;
            std::filesystem::path GenerateFullMaterialTexturePath(const Material& material, const std::filesystem::path& matetrialTexturePath) const;

            std::filesystem::path SceneFilePath;
            std::filesystem::path MeshFolderPath;
            std::filesystem::path MaterialFolderPath;
        };

        void LoadUtilityResources(const std::filesystem::path& executableFolder);
        void SerializeMeshDataIfNeeded(Mesh& mesh, const FileStructure& fileStructure);
        void SerializeMaterialDataIfNeeded(Material& material, const FileStructure& fileStructure);
        std::string EnsureMeshNameUniqueness(const std::string& meshName);
        std::string EnsureMaterialNameUniqueness(const std::string& materialName);
        std::string EnsureNameUniqueness(const std::string& name, robin_hood::unordered_flat_set<std::string>& set);

        std::list<Mesh> mMeshes;
        std::list<MeshInstance> mMeshInstances;
        std::list<Material> mMaterials;
        std::list<FlatLight> mRectangularLights;
        std::list<FlatLight> mDiskLights;
        std::list<SphericalLight> mSphericalLights;

        robin_hood::unordered_flat_set<std::string> mMeshNames;
        robin_hood::unordered_flat_set<std::string> mMaterialNames;

        Camera mCamera;
        LuminanceMeter mLuminanceMeter;
        GTTonemappingParameterss mTonemappingParams;
        BloomParameters mBloomParameters;
        GIManager mGIManager;
        Sky mSky;

        Memory::GPUResourceProducer::TexturePtr mBlueNoiseTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAAAreaTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAASearchTexture;
        Mesh mUnitCube;
        Mesh mUnitSphere;
        ThirdPartySceneLoader mThirdPartySceneLoader;
        MaterialLoader mMaterialLoader;

        Memory::GPUResourceProducer* mResourceProducer;
        SceneGPUStorage mGPUStorage;

        std::vector<MeshInstance*> mMeshInstanceGPUIndexMappings;
        std::vector<LightVariant> mLightGPUIndexMappings;

        uint64_t mTotalVertexCount = 0;
        uint64_t mTotalIndexCount = 0;

    public:
        inline Camera& GetMainCamera() { return mCamera; }
        inline const Camera& GetMainCamera() const { return mCamera; }
        inline LuminanceMeter& GetLuminanceMeter() { return mLuminanceMeter; }
        inline const LuminanceMeter& GetLuminanceMeter() const { return mLuminanceMeter; }
        inline GIManager& GetGIManager() { return mGIManager; }
        inline const GIManager& GetGIManager() const { return mGIManager; }
        inline Sky& GetSky() { return mSky; }
        inline const Sky& GetSky() const { return mSky; }
        inline const auto& GetMeshes() const { return mMeshes; }
        inline const auto& GetMeshInstances() const { return mMeshInstances; }
        inline const auto& GetMaterials() const { return mMaterials; }
        inline const auto& GetRectangularLights() const { return mRectangularLights; }
        inline const auto& GetDiskLights() const { return mDiskLights; }
        inline const auto& GetSphericalLights() const { return mSphericalLights; }
        inline const auto& GetTonemappingParams() const { return mTonemappingParams; }
        inline const auto& GetBloomParams() const { return mBloomParameters; }

        inline auto& GetMeshes() { return mMeshes; }
        inline auto& GetMeshInstances() { return mMeshInstances; }
        inline auto& GetMaterials() { return mMaterials; }
        inline auto& GetRectangularLights() { return mRectangularLights; }
        inline auto& GetDiskLights() { return mDiskLights; }
        inline auto& GetSphericalLights() { return mSphericalLights; }
        inline auto& GetTonemappingParams() { return mTonemappingParams; }
        inline auto& GetBloomParams() { return mBloomParameters; }

        inline auto GetTotalLightCount() const { return mRectangularLights.size() + mDiskLights.size() + mSphericalLights.size() + 1 /*Sun*/; }

        inline const auto GetBlueNoiseTexture() const { return mBlueNoiseTexture.get(); }
        inline const auto GetSMAASearchTexture() const { return mSMAASearchTexture.get(); }
        inline const auto GetSMAAAreaTexture() const { return mSMAAAreaTexture.get(); }
        inline const Mesh& GetUnitCube() const { return mUnitCube; }
        inline const Mesh& GetUnitSphere() const { return mUnitSphere; }

        inline MeshInstance* GetMeshInstanceForGPUIndex(uint64_t index) const { return mMeshInstanceGPUIndexMappings[index]; }
        inline LightVariant GetLightForGPUIndex(uint64_t index) const { return mLightGPUIndexMappings[index]; }

        inline SceneGPUStorage& GetGPUStorage() { return mGPUStorage; }

        inline const auto GetTotalVertexCount() const { return mTotalVertexCount; }
        inline const auto GetTotalIndexCount() const { return mTotalIndexCount; }
    };

}
