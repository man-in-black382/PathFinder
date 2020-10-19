#include "Scene.hpp"

namespace PathFinder 
{

    Scene::Scene(const std::filesystem::path& executableFolder, Memory::GPUResourceProducer* resourceProducer)
        : mResourceLoader{ executableFolder, resourceProducer }, mMeshLoader{ executableFolder }
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

    std::optional<Scene::EntityVariant> Scene::GetEntityByID(const EntityID& id) const
    {
        auto it = mMappedEntities.find(id);
        return it == mMappedEntities.end() ? std::nullopt : std::optional(it->second);
    }

    void Scene::RemapEntityIDs()
    {
        mMappedEntities.clear();

        auto remap = [this](auto&& entities)
        {
            for (auto& entity : entities)
            {
                mMappedEntities[entity.ID()] = &entity;
            }
        };

        remap(mMeshInstances);
        remap(mSphericalLights);
        remap(mDiskLights);
        remap(mRectangularLights);
    }

    void Scene::LoadUtilityResources()
    {
        mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/BlueNoise3DIndependent.dds");
        mSMAAAreaTexture = mResourceLoader.LoadTexture("/Precompiled/SMAAAreaTex.dds");
        mSMAASearchTexture = mResourceLoader.LoadTexture("/Precompiled/SMAASearchTex.dds");
        mUnitCube = mMeshLoader.Load("Precompiled/UnitCube.obj").back();
        //mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/BlueNoise3D16.dds");
        //mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/Blue_Noise_RGBA_0.dds");
    }

}
