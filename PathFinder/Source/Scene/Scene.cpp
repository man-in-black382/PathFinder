#include "Scene.hpp"

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/list.h>
#include <bitsery/traits/vector.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/adapter/stream.h>
#include <bitsery/ext/pointer.h>

#include <fstream>

#include <Foundation/Filesystem.hpp>
#include <Foundation/StringUtils.hpp>

namespace PathFinder 
{

    Scene::Scene(
        const std::filesystem::path& executableFolder,
        const HAL::Device* device,
        Memory::GPUResourceProducer* resourceProducer,
        const PipelineResourceStorage* pipelineResourceStorage,
        const RenderSurfaceDescription* renderSurfaceDescription,
        const RenderSettings* renderSettings)
        : 
        mResourceProducer{ resourceProducer },
        mLuminanceMeter{ &mCamera },
        mGPUStorage{ this, device, resourceProducer, pipelineResourceStorage, renderSurfaceDescription, renderSettings },
        mMaterialLoader{ executableFolder, resourceProducer },
        mGIManager{ this }
    {
        LoadUtilityResources(executableFolder);
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

    void Scene::MapEntitiesToGPUIndices()
    {
        mMeshInstanceGPUIndexMappings.resize(mMeshInstances.size());
        mLightGPUIndexMappings.resize(mRectangularLights.size() + mDiskLights.size() + mSphericalLights.size());

        for (MeshInstance& instance : mMeshInstances)
            mMeshInstanceGPUIndexMappings[instance.GetIndexInGPUTable()] = &instance;

        for (FlatLight& light : mRectangularLights)
            mLightGPUIndexMappings[light.GetIndexInGPUTable()] = &light;

        for (FlatLight& light : mDiskLights)
            mLightGPUIndexMappings[light.GetIndexInGPUTable()] = &light;

        for (SphericalLight& light : mSphericalLights)
            mLightGPUIndexMappings[light.GetIndexInGPUTable()] = &light;
    }

    void Scene::UpdatePreviousFrameValues()
    {
        for (MeshInstance& instance : mMeshInstances)
            instance.UpdatePreviousFrameValues();

        for (FlatLight& light : mRectangularLights)
            light.UpdatePreviousFrameValues();

        for (FlatLight& light : mDiskLights)
            light.UpdatePreviousFrameValues();

        for (SphericalLight& light : mSphericalLights)
            light.UpdatePreviousFrameValues();
    }

    void Scene::LoadThirdPartyScene(const std::filesystem::path& path)
    {
        std::vector<ThirdPartySceneLoader::LoadedMesh>& loadedMeshes = mThirdPartySceneLoader.Load(path);
        std::vector<Material*> insertedMaterials;

        for (Material& material : mThirdPartySceneLoader.LoadedMaterials())
        {
            Material* insertedMaterial = &mMaterials.emplace_back(std::move(material));
            insertedMaterial->Name = EnsureMaterialNameUniqueness(insertedMaterial->Name);
            insertedMaterials.push_back(insertedMaterial);

            mMaterialLoader.LoadMaterial(*insertedMaterial);
        }
            
        for (ThirdPartySceneLoader::LoadedMesh& loadedMesh : loadedMeshes)
        {
            Mesh* insertedMesh = &mMeshes.emplace_back(std::move(loadedMesh.MeshObject));
            Material* material = insertedMaterials[loadedMesh.MaterialIndex];
            insertedMesh->SetName(EnsureMeshNameUniqueness(insertedMesh->GetName()));
            mMeshInstances.emplace_back(insertedMesh, material);

            mTotalVertexCount += insertedMesh->GetVertices().size();
            mTotalIndexCount += insertedMesh->GetIndices().size();
        }
    }

    void Scene::Serialize(const std::filesystem::path& destination)
    {
        FileStructure sceneFiles{ destination };
        sceneFiles.CreateDirectories();

        for (Mesh& mesh : mMeshes)
            SerializeMeshDataIfNeeded(mesh, sceneFiles);

        for (Material& material : mMaterials)
            SerializeMaterialDataIfNeeded(material, sceneFiles);

        std::ofstream stream{ destination, std::ios::out | std::ios::trunc | std::ios::binary };
        assert_format(stream.is_open(), "File (", destination.string(), ") couldn't be opened for writing");

        using Serializer = bitsery::Serializer<bitsery::OutputBufferedStreamAdapter, bitsery::ext::PointerLinkingContext>;

        bitsery::ext::PointerLinkingContext context{};
        Serializer serializer{ context, stream };

        serializer.object(mCamera);
        serializer.container(mMeshes, std::numeric_limits<uint64_t>::max(), [](Serializer& s, Mesh& m) { s.ext(m, bitsery::ext::ReferencedByPointer{}); });
        serializer.container(mMaterials, std::numeric_limits<uint64_t>::max(), [](Serializer& s, Material& m) { s.ext(m, bitsery::ext::ReferencedByPointer{}); });
        serializer.container(mMeshInstances, std::numeric_limits<uint64_t>::max());

        serializer.adapter().flush();
        stream.close();

        assert_format(context.isValid(), "Scene serialization failed");
    }

    void Scene::Deserialize(const std::filesystem::path& source)
    {
        FileStructure sceneFiles{ source };

        std::fstream stream{ source, std::ios::binary | std::ios::in };
        assert_format(stream.is_open(), "File (", source.string(), ") couldn't be opened for reading");

        using Deserializer = bitsery::Deserializer<bitsery::InputStreamAdapter, bitsery::ext::PointerLinkingContext>;

        bitsery::ext::PointerLinkingContext context{};
        Deserializer deserializer{ context, stream };

        deserializer.object(mCamera);
        deserializer.container(mMeshes, std::numeric_limits<uint64_t>::max(), [](Deserializer& s, Mesh& m) { s.ext(m, bitsery::ext::ReferencedByPointer{}); });
        deserializer.container(mMaterials, std::numeric_limits<uint64_t>::max(), [](Deserializer& s, Material& m) { s.ext(m, bitsery::ext::ReferencedByPointer{}); });
        deserializer.container(mMeshInstances, std::numeric_limits<uint64_t>::max());

        stream.close();

        assert_format(context.isValid(), "Scene deserialization failed");

        for (Mesh& mesh : mMeshes)
        {
            mesh.DeserializeVertexData(sceneFiles.MeshFolderPath / (mesh.GetName() + ".pfmeshdat"));
            mesh.SetName(EnsureMeshNameUniqueness(mesh.GetName()));
        }

        for (Material& material : mMaterials)
        {
            material.DeserializeTextures(sceneFiles.MaterialFolderPath / (material.Name + ".pfmatdat"), mResourceProducer);
            material.Name = EnsureMaterialNameUniqueness(material.Name);
            mMaterialLoader.SetCommonMaterialTextures(material);
        }
    }

    void Scene::LoadUtilityResources(const std::filesystem::path& executableFolder)
    {
        ResourceLoader resourceLoader{ mResourceProducer };

        mBlueNoiseTexture = resourceLoader.LoadTexture(executableFolder / "Precompiled" / "BlueNoise3DIndependent.dds");
        mSMAAAreaTexture = resourceLoader.LoadTexture(executableFolder / "Precompiled" / "SMAAAreaTex.dds");
        mSMAASearchTexture = resourceLoader.LoadTexture(executableFolder / "Precompiled" / "SMAASearchTex.dds");
        mUnitCube = std::move(mThirdPartySceneLoader.Load(executableFolder / "Precompiled" / "UnitCube.obj").back().MeshObject);
        mUnitSphere = std::move(mThirdPartySceneLoader.Load(executableFolder / "Precompiled" / "UnitSphere.obj").back().MeshObject);
    }

    void Scene::SerializeMeshDataIfNeeded(Mesh& mesh, const FileStructure& fileStructure)
    {
        std::filesystem::path meshAbsolutePath;

        if (mesh.GetName().empty())
        {
            std::string meshName = RandomAlphanumericString(8);
            meshAbsolutePath = fileStructure.MeshFolderPath / (meshName + ".pfmeshdat");
            meshAbsolutePath = Foundation::AppendUniquePostfixIfPathExists(meshAbsolutePath);
            mesh.SetName(meshAbsolutePath.filename().replace_extension("").string());
        }
        else
        {
            meshAbsolutePath = fileStructure.MeshFolderPath / (mesh.GetName() + ".pfmeshdat");
        }

        if (!std::filesystem::exists(meshAbsolutePath))
            mesh.SerializeVertexData(meshAbsolutePath);
    }

    void Scene::SerializeMaterialDataIfNeeded(Material& material, const FileStructure& fileStructure)
    {
        std::filesystem::path materialAbsolutePath;

        if (material.Name.empty())
        {
            std::string materialName = RandomAlphanumericString(8);
            materialAbsolutePath = fileStructure.MaterialFolderPath / (materialName + ".pfmatdat");
            materialAbsolutePath = Foundation::AppendUniquePostfixIfPathExists(materialAbsolutePath);
            material.Name = materialAbsolutePath.filename().replace_extension("").string();
        }
        else
        {
            materialAbsolutePath = fileStructure.MaterialFolderPath / (material.Name + ".pfmatdat");
        }

        if (!std::filesystem::exists(materialAbsolutePath))
            material.SerializeTextures(materialAbsolutePath);
    }

    std::string Scene::EnsureMeshNameUniqueness(const std::string& meshName)
    {
        return EnsureNameUniqueness(meshName.empty() ? "Mesh" : meshName, mMeshNames);
    }

    std::string Scene::EnsureMaterialNameUniqueness(const std::string& materialName)
    {
        return EnsureNameUniqueness(materialName.empty() ? "Material" : materialName, mMaterialNames);
    }

    std::string Scene::EnsureNameUniqueness(const std::string& name, robin_hood::unordered_flat_set<std::string>& set)
    {
        std::string candidate = name;
        uint64_t counter = 1;

        while (set.contains(candidate))
        {
            candidate = name + "_" + std::to_string(counter);
            ++counter;
        }

        set.insert(candidate);

        return candidate;
    }

    Scene::FileStructure::FileStructure(const std::filesystem::path& sceneFilePath)
    {
        std::filesystem::path sceneFileName = sceneFilePath.filename().replace_extension("");

        std::filesystem::path folder = sceneFilePath;
        folder.remove_filename();

        SceneFilePath = folder / (sceneFileName.string() + ".pfscene");
        MeshFolderPath = folder / (sceneFileName.string() + "_Meshes");
        MaterialFolderPath = folder / (sceneFileName.string() + "_Materials");
    }

    void Scene::FileStructure::CreateDirectories() const
    {
        std::filesystem::create_directories(MeshFolderPath);
        std::filesystem::create_directories(MaterialFolderPath);
    }

    std::filesystem::path Scene::FileStructure::GenerateFullMaterialTexturePath(const Material& material, const std::filesystem::path& matetrialTexturePath) const
    {
        return MaterialFolderPath / material.Name / matetrialTexturePath.filename();
    }

}
