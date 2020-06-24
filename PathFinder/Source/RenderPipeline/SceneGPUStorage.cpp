#include "SceneGPUStorage.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    SceneGPUStorage::SceneGPUStorage(Scene* scene, const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer)
        : mScene{ scene }, mDevice{ device }, mResourceProducer{ resourceProducer }, mTopAccelerationStructure{ device, resourceProducer }
    {
        mTopAccelerationStructure.SetDebugName("All Meshes Top RT AS");
    }        

    void SceneGPUStorage::UploadMeshes()
    {
        auto& meshes = mScene->Meshes();

        mBottomAccelerationStructures.clear();

        for (Mesh& mesh : meshes)
        {
            VertexStorageLocation locationInStorage = WriteToTemporaryBuffers(
                mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());

            mesh.SetVertexStorageLocation(locationInStorage);
        }

        SubmitTemporaryBuffersToGPU<Vertex1P1N1UV1T1BT>();
    }

    void SceneGPUStorage::UploadMaterials()
    {
        auto& materials = mScene->Materials();

        if (!mMaterialTable || mMaterialTable->Capacity<GPUMaterialTableEntry>() < materials.size())
        {
            HAL::Buffer::Properties<GPUMaterialTableEntry> props{ materials.size() };
            mMaterialTable = mResourceProducer->NewBuffer(props);
            mMaterialTable->SetDebugName("Material Table");
        }

        mMaterialTable->RequestWrite();

        uint64_t materialIndex = 0;

        for (Material& material : materials)
        {
            // All ltc look-up tables are expected to be of the same size
            auto lut0SpecularSize = material.LTC_LUT_MatrixInverse_Specular->HALTexture()->Dimensions();

            GPUMaterialTableEntry materialEntry{
                material.AlbedoMap->GetSRDescriptor()->IndexInHeapRange(),
                material.NormalMap->GetSRDescriptor()->IndexInHeapRange(),
                material.RoughnessMap->GetSRDescriptor()->IndexInHeapRange(),
                material.MetalnessMap->GetSRDescriptor()->IndexInHeapRange(),
                material.AOMap->GetSRDescriptor()->IndexInHeapRange(),
                material.DisplacementMap->GetSRDescriptor()->IndexInHeapRange(),
                material.DistanceField->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_MatrixInverse_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Matrix_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Terms_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_MatrixInverse_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Matrix_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Terms_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                lut0SpecularSize.Width
            };

            material.GPUMaterialTableIndex = materialIndex;

            mMaterialTable->Write(&materialEntry, materialIndex, 1);
            ++materialIndex;
        }
    }

    void SceneGPUStorage::UploadMeshInstances()
    {
        auto& meshInstances = mScene->MeshInstances();

        auto requiredBufferSize = meshInstances.size();

        if (!mMeshInstanceTable || mMeshInstanceTable->Capacity<GPUMeshInstanceTableEntry>() < requiredBufferSize)
        {
            HAL::Buffer::Properties<GPUMeshInstanceTableEntry> props{ requiredBufferSize };
            mMeshInstanceTable = mResourceProducer->NewBuffer(props, Memory::GPUResource::UploadStrategy::DirectAccess);
            mMeshInstanceTable->SetDebugName("Mesh Instance Table");
        }

        mMeshInstanceTable->RequestWrite();
        mTopAccelerationStructure.Clear();

        uint32_t index = 0;

        for (MeshInstance& instance : meshInstances)
        {
            GPUMeshInstanceTableEntry instanceEntry{
                instance.Transformation().ModelMatrix(),
                instance.Transformation().NormalMatrix(),
                instance.AssosiatedMaterial()->GPUMaterialTableIndex,
                instance.AssosiatedMesh()->LocationInVertexStorage().VertexBufferOffset,
                instance.AssosiatedMesh()->LocationInVertexStorage().IndexBufferOffset,
                instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount
            };

            BottomRTAS& blas = mBottomAccelerationStructures[instance.AssosiatedMesh()->LocationInVertexStorage().BottomAccelerationStructureIndex];
            mTopAccelerationStructure.AddInstance(blas, index, instance.Transformation().ModelMatrix());
            instance.SetGPUInstanceIndex(index);

            mMeshInstanceTable->Write(&instanceEntry, index, 1);

            ++index;
        }

        mTopAccelerationStructure.Build();
    }

    void SceneGPUStorage::UploadLights()
    {
        auto& sphericalLights = mScene->SphericalLights();
        auto& rectangularLights = mScene->RectangularLights();
        auto& diskLights = mScene->DiskLights();

        auto requiredBufferSize = sphericalLights.size() + rectangularLights.size() + diskLights.size();

        if (!mLightTable || mLightTable->Capacity<GPULightTableEntry>() < requiredBufferSize)
        {
            HAL::Buffer::Properties<GPULightTableEntry> props{ requiredBufferSize };
            mLightTable = mResourceProducer->NewBuffer(props, Memory::GPUResource::UploadStrategy::DirectAccess);
            mLightTable->SetDebugName("Lights Instance Table");
        }

        mLightTable->RequestWrite();

        mLightsMaximumLuminance = 0.0;

        uint32_t index = 0;
        mLightTablePartitionInfo = {};
        mLightTablePartitionInfo.TotalLightsCount = requiredBufferSize;
        mLightTablePartitionInfo.SphericalLightsOffset = index;

        for (SphericalLight& light : sphericalLights)
        {
            GPULightTableEntry lightEntry = CreateLightGPUTableEntry(light);
            mLightTable->Write(&lightEntry, index, 1);
            light.SetGPULightTableIndex(index);

            ++index;
            ++mLightTablePartitionInfo.SphericalLightsCount;

            mLightsMaximumLuminance += std::max(light.Color().R() * light.Luminance(), 
                std::max(light.Color().G() * light.Luminance(), light.Color().B() * light.Luminance()));
        }

        mLightTablePartitionInfo.RectangularLightsOffset = index;

        for (FlatLight& light : rectangularLights)
        {
            GPULightTableEntry lightEntry = CreateLightGPUTableEntry(light);
            mLightTable->Write(&lightEntry, index, 1);
            light.SetGPULightTableIndex(index);

            ++index;
            ++mLightTablePartitionInfo.RectangularLightsCount;

            mLightsMaximumLuminance += std::max(light.Color().R() * light.Luminance(),
                std::max(light.Color().G() * light.Luminance(), light.Color().B() * light.Luminance()));
        }

        mLightTablePartitionInfo.EllipticalLightsOffset = index;

        for (FlatLight& light : diskLights)
        {
            GPULightTableEntry lightEntry = CreateLightGPUTableEntry(light);
            mLightTable->Write(&lightEntry, index, 1);
            light.SetGPULightTableIndex(index);

            ++index;
            ++mLightTablePartitionInfo.EllipticalLightsCount;

            mLightsMaximumLuminance += std::max(light.Color().R() * light.Luminance(),
                std::max(light.Color().G() * light.Luminance(), light.Color().B() * light.Luminance()));
        }
    }

    GPUCamera SceneGPUStorage::CameraGPURepresentation() const
    {
        const PathFinder::Camera& camera = mScene->MainCamera();

        GPUCamera gpuCamera{};

        gpuCamera.Position = glm::vec4{ camera.Position(), 1.0 };
        gpuCamera.View = camera.View();
        gpuCamera.Projection = camera.Projection();
        gpuCamera.ViewProjection = camera.ViewProjection();
        gpuCamera.InverseView = camera.InverseView();
        gpuCamera.InverseProjection = camera.InverseProjection();
        gpuCamera.InverseViewProjection = camera.InverseViewProjection();
        gpuCamera.ExposureValue100 = camera.ExposureValue100();
        gpuCamera.FarPlane = camera.FarClipPlane();
        gpuCamera.NearPlane = camera.NearClipPlane();

        return gpuCamera;
    }

    GPULightTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const FlatLight& light) const
    {
        GPULightTableEntry::LightType lightType{};

        switch (light.LightType())
        {
        case FlatLight::Type::Disk: lightType = GPULightTableEntry::LightType::Disk; break;
        case FlatLight::Type::Rectangle: lightType = GPULightTableEntry::LightType::Rectangle; break;
        }

        return{
                glm::vec4(light.Normal(), 0.0f),
                glm::vec4(light.Position(), 1.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f),
                light.Luminance(),
                light.Width(),
                light.Height(),
                std::underlying_type_t<GPULightTableEntry::LightType>(lightType)
        };
    }

    GPULightTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const SphericalLight& light) const
    {
        return{
                glm::vec4(0.0f), // No orientation required for spherical lights
                glm::vec4(light.Position(), 1.0f),
                glm::vec4(light.Color().R(), light.Color().G(), light.Color().B(), 0.0f),
                light.Luminance(),
                light.Radius(),
                light.Radius(),
                std::underlying_type_t<GPULightTableEntry::LightType>(GPULightTableEntry::LightType::Sphere)
        };
    }

}
