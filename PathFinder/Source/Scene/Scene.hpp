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

        Scene(const std::filesystem::path& executableFolder, Memory::GPUResourceProducer* resourceProducer);

        Mesh& AddMesh(Mesh&& mesh);
        MeshInstance& AddMeshInstance(MeshInstance&& instance);
        Material& AddMaterial(Material&& material);
        FlatLightIt EmplaceDiskLight();
        FlatLightIt EmplaceRectangularLight();
        SphericalLightIt EmplaceSphericalLight();

    private:
        void LoadUtilityResources();

        std::list<Mesh> mMeshes;
        std::list<MeshInstance> mMeshInstances;
        std::list<Material> mMaterials;
        std::list<FlatLight> mRectangularLights;
        std::list<FlatLight> mDiskLights;
        std::list<SphericalLight> mSphericalLights;

        Camera mCamera;
        GTTonemappingParameterss mTonemappingParams;
        BloomParameters mBloomParameters;

        Memory::GPUResourceProducer::TexturePtr mBlueNoiseTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAAAreaTexture;
        Memory::GPUResourceProducer::TexturePtr mSMAASearchTexture;

        ResourceLoader mResourceLoader;

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

        inline const auto BlueNoiseTexture() const { return mBlueNoiseTexture.get(); }
        inline const auto SMAASearchTexture() const { return mSMAASearchTexture.get(); }
        inline const auto SMAAAreaTexture() const { return mSMAAAreaTexture.get(); }
    };

}
