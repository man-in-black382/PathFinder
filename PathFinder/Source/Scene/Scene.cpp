#include "Scene.hpp"

namespace PathFinder 
{

    Scene::Scene(const std::filesystem::path& executableFolder, Memory::GPUResourceProducer* resourceProducer)
        : mResourceLoader{ executableFolder, resourceProducer } 
    {
        LoadUtilityResources();
    }

    Mesh& Scene::AddMesh(Mesh&& mesh)
    {
        mMeshes.emplace_back(std::move(mesh));
        return mMeshes.back();
    }

    MeshInstance& Scene::AddMeshInstance(MeshInstance&& instance)
    {
        mMeshInstances.emplace_back(std::move(instance));
        return mMeshInstances.back();
    }

    Material& Scene::AddMaterial(Material&& material)
    {
        mMaterials.emplace_back(std::move(material));
        return mMaterials.back();
    }

    Scene::FlatLightIt Scene::EmplaceDiskLight()
    {
        mDiskLights.emplace_back(FlatLight::Type::Disk);
        return std::prev(mDiskLights.end());
    }

    Scene::FlatLightIt Scene::EmplaceRectangularLight()
    {
        mRectangularLights.emplace_back(FlatLight::Type::Rectangle);
        return std::prev(mRectangularLights.end());
    }

    Scene::SphericalLightIt Scene::EmplaceSphericalLight()
    {
        mSphericalLights.emplace_back();
        return std::prev(mSphericalLights.end());
    }

    void Scene::LoadUtilityResources()
    {
        mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/BlueNoise3DIndependent.dds");
        mSMAAAreaTexture = mResourceLoader.LoadTexture("/Precompiled/SMAAAreaTex.dds");
        mSMAASearchTexture = mResourceLoader.LoadTexture("/Precompiled/SMAASearchTex.dds");
        //mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/BlueNoise3D16.dds");
        //mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/Blue_Noise_RGBA_0.dds");
    }

}
