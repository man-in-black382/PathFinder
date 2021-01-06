#include "Scene.hpp"

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>

#include <fstream>

namespace PathFinder 
{

    Scene::Scene(const std::filesystem::path& executableFolder, const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer)
        : mResourceLoader{ executableFolder, resourceProducer }, mMeshLoader{ executableFolder }, mLuminanceMeter{ &mCamera }, mGPUStorage{ this, device, resourceProducer }
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

    void Scene::Serialize(const std::filesystem::path& destination) const
    {
        using Buffer = std::vector<uint8_t>;
        using Writer = bitsery::OutputBufferAdapter<Buffer>;
        using Reader = bitsery::InputBufferAdapter<Buffer>;

        Buffer buffer{};
        size_t writtenSize{};

        bitsery::ext::PointerLinkingContext ctx{};
        writtenSize = bitsery::quickSerialization(ctx, Writer{ buffer }, mCamera);

        //make sure that pointer linking context is valid
        //this ensures that all non-owning pointers points to data that has been serialized,
        //so we can successfully reconstruct pointers after deserialization
        assert(ctx.isValid());

        std::filesystem::path directory = destination;
        directory.remove_filename();

        std::filesystem::create_directories(directory);

        std::ofstream sceneFile{ destination, std::ios::out | std::ios::binary };

        if (sceneFile)
        {
            sceneFile.write((const char*)buffer.data(), writtenSize);
            sceneFile.close();
        }
    }

    void Scene::Deserialize(const std::filesystem::path& source)
    {

    }

    void Scene::LoadUtilityResources()
    {
        mBlueNoiseTexture = mResourceLoader.LoadTexture("/Precompiled/BlueNoise3DIndependent.dds");
        mSMAAAreaTexture = mResourceLoader.LoadTexture("/Precompiled/SMAAAreaTex.dds");
        mSMAASearchTexture = mResourceLoader.LoadTexture("/Precompiled/SMAASearchTex.dds");
        mUnitCube = mMeshLoader.Load("Precompiled/UnitCube.obj").back();
        mUnitSphere = mMeshLoader.Load("Precompiled/UnitSphere.obj").back();
    }

}
