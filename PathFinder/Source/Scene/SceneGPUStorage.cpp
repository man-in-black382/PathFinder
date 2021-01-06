#include "SceneGPUStorage.hpp"
#include "EntityID.hpp"
#include "Scene.hpp"

#include <algorithm>
#include <iterator>

#include <RenderPipeline/DrawablePrimitive.hpp>
#include <fplus/fplus.hpp>

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
            assert_format(!mesh.Vertices().empty(), "Empty meshes are not allowed");

            VertexStorageLocation locationInStorage = WriteToTemporaryBuffers(
                mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());

            mesh.SetVertexStorageLocation(locationInStorage);
        }

        auto quadVertices = fplus::transform([](const glm::vec3& p) { return Vertex1P1N1UV1T1BT{ glm::vec4{p, 1.0f} }; }, DrawablePrimitive::UnitQuadVertices);

        mUnitQuadVertexLocation = WriteToTemporaryBuffers(
            quadVertices.data(), quadVertices.size(),
            DrawablePrimitive::UnitQuadIndices.data(), DrawablePrimitive::UnitQuadIndices.size());

        mUnitCubeVertexLocation = WriteToTemporaryBuffers(
            mScene->UnitCube().Vertices().data(), mScene->UnitCube().Vertices().size(), 
            mScene->UnitCube().Indices().data(), mScene->UnitCube().Indices().size());

        mUnitSphereVertexLocation = WriteToTemporaryBuffers(
            mScene->UnitSphere().Vertices().data(), mScene->UnitSphere().Vertices().size(),
            mScene->UnitSphere().Indices().data(), mScene->UnitSphere().Indices().size());

        SubmitTemporaryBuffersToGPU<Vertex1P1N1UV1T1BT>();
    }

    void SceneGPUStorage::UploadMaterials()
    {
        auto& materials = mScene->Materials();

        if (materials.empty()) return;

        if (!mMaterialTable || mMaterialTable->Capacity<GPUMaterialTableEntry>() < materials.size())
        {
            auto properties = HAL::BufferProperties::Create<GPUMaterialTableEntry>(materials.size());
            mMaterialTable = mResourceProducer->NewBuffer(properties);
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

    void SceneGPUStorage::UploadInstances()
    {
        mUniqueEntityID = 0;
        mTopAccelerationStructure.Clear();
        UploadMeshInstances();
        UploadLights();
        mTopAccelerationStructure.Build();
    }

    void SceneGPUStorage::UploadMeshInstances()
    {
        auto& meshInstances = mScene->MeshInstances();
        auto& sphericalLights = mScene->SphericalLights();
        auto& rectangularLights = mScene->RectangularLights();
        auto& diskLights = mScene->DiskLights();

        auto requiredBufferSize = meshInstances.size() + mScene->TotalLightCount();

        if (requiredBufferSize == 0)
            return;

        if (!mMeshInstanceTable || mMeshInstanceTable->Capacity<GPUMeshInstanceTableEntry>() < requiredBufferSize)
        {
            auto properties = HAL::BufferProperties::Create<GPUMeshInstanceTableEntry>(requiredBufferSize);
            mMeshInstanceTable = mResourceProducer->NewBuffer(properties, Memory::GPUResource::AccessStrategy::DirectUpload);
            mMeshInstanceTable->SetDebugName("Mesh Instance Table");
        }

        mMeshInstanceTable->RequestWrite();

        uint32_t instanceIdx = 0;

        auto uploadInstance = [&](const GPUMeshInstanceTableEntry& gpuInstance, const VertexStorageLocation& vertexLocations, EntityMask mask, auto&& sceneObject)
        {
            EntityID entityId = GetNextEntityID();

            sceneObject.SetIndexInGPUTable(instanceIdx);
            sceneObject.SetEntityID(entityId);

            BottomRTAS& blas = mBottomAccelerationStructures[vertexLocations.BottomAccelerationStructureIndex];
            mTopAccelerationStructure.AddInstance(blas, RTASInstanceInfoForEntity(entityId, mask), gpuInstance.InstanceWorldMatrix);
            mMeshInstanceTable->Write(&gpuInstance, instanceIdx, 1);

            ++instanceIdx;
        };

        for (MeshInstance& instance : meshInstances)
        {
            GPUMeshInstanceTableEntry instanceEntry{
                instance.Transformation().ModelMatrix(),
                instance.PrevTransformation().ModelMatrix(),
                instance.Transformation().NormalMatrix(),
                instance.AssociatedMaterial()->GPUMaterialTableIndex,
                instance.AssociatedMesh()->LocationInVertexStorage().VertexBufferOffset,
                instance.AssociatedMesh()->LocationInVertexStorage().IndexBufferOffset,
                instance.AssociatedMesh()->LocationInVertexStorage().IndexCount,
                instance.AssociatedMesh()->HasTangentSpace()
            };

            uploadInstance(instanceEntry, instance.AssociatedMesh()->LocationInVertexStorage(), EntityMask::MeshInstance, instance);
            instance.UpdatePreviousTransform();
        }
    }

    void SceneGPUStorage::UploadLights()
    {
        auto& sphericalLights = mScene->SphericalLights();
        auto& rectangularLights = mScene->RectangularLights();
        auto& diskLights = mScene->DiskLights();

        auto requiredBufferSize = mScene->TotalLightCount();

        if (!mLightTable || mLightTable->Capacity<GPULightTableEntry>() < requiredBufferSize)
        {
            auto properties = HAL::BufferProperties::Create<GPULightTableEntry>(requiredBufferSize);
            mLightTable = mResourceProducer->NewBuffer(properties, Memory::GPUResource::AccessStrategy::DirectUpload);
            mLightTable->SetDebugName("Lights Instance Table");
        }

        mLightTable->RequestWrite();

        uint32_t index = 0;
        mLightTablePartitionInfo = {};
        mLightTablePartitionInfo.TotalLightsCount = 0;
        mLightTablePartitionInfo.SphericalLightsOffset = index;

        auto uploadLights = [this, &index](auto&& lights, uint32_t& tableOffset, uint32_t& lightCount, const VertexStorageLocation& vertexLocation)
        {
            tableOffset = index;

            for (auto& light : lights)
            {
                if (light.LuminousPower() <= 0.0) continue;

                GPULightTableEntry lightEntry = CreateLightGPUTableEntry(light);
                mLightTable->Write(&lightEntry, index, 1);

                EntityID entityId = GetNextEntityID();

                light.SetEntityID(entityId);
                light.SetIndexInGPUTable(index);
                light.SetVertexStorageLocation(vertexLocation);

                BottomRTAS& blas = mBottomAccelerationStructures[vertexLocation.BottomAccelerationStructureIndex];
                mTopAccelerationStructure.AddInstance(blas, RTASInstanceInfoForEntity(entityId, EntityMask::Light), light.ModelMatrix());

                ++index;
                ++lightCount;
                ++mLightTablePartitionInfo.TotalLightsCount;
            }
        };

        uploadLights(mScene->SphericalLights(), mLightTablePartitionInfo.SphericalLightsOffset, mLightTablePartitionInfo.SphericalLightsCount, mUnitSphereVertexLocation);
        uploadLights(mScene->RectangularLights(), mLightTablePartitionInfo.RectangularLightsOffset, mLightTablePartitionInfo.RectangularLightsCount, mUnitQuadVertexLocation);
        uploadLights(mScene->DiskLights(), mLightTablePartitionInfo.EllipticalLightsOffset, mLightTablePartitionInfo.EllipticalLightsCount, mUnitQuadVertexLocation);
    }

    EntityID SceneGPUStorage::GetNextEntityID()
    {
        return ++mUniqueEntityID;
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
        gpuCamera.FoVH = glm::radians(camera.FOVH());
        gpuCamera.FoVV = glm::radians(camera.FOVV());
        gpuCamera.FoVHTan = tan(gpuCamera.FoVH);
        gpuCamera.FoVVTan = tan(gpuCamera.FoVV);
        gpuCamera.AspectRatio = camera.AspectRatio();

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
                std::underlying_type_t<GPULightTableEntry::LightType>(lightType),
                light.ModelMatrix(),
                mUnitQuadVertexLocation.VertexBufferOffset,
                mUnitQuadVertexLocation.IndexBufferOffset,
                mUnitQuadVertexLocation.IndexCount
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
                std::underlying_type_t<GPULightTableEntry::LightType>(GPULightTableEntry::LightType::Sphere),
                light.ModelMatrix(),
                mUnitSphereVertexLocation.VertexBufferOffset,
                mUnitSphereVertexLocation.IndexBufferOffset,
                mUnitSphereVertexLocation.IndexCount
        };
    }

}
