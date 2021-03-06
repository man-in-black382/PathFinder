#include "SceneGPUStorage.hpp"
#include "Scene.hpp"

#include <algorithm>
#include <iterator>

#include <RenderPipeline/DrawablePrimitive.hpp>
#include <RenderPipeline/RenderPasses/PipelineNames.hpp>
#include <fplus/fplus.hpp>

namespace PathFinder
{

    SceneGPUStorage::SceneGPUStorage(
        Scene* scene, 
        const HAL::Device* device,
        Memory::GPUResourceProducer* resourceProducer, 
        const PipelineResourceStorage* pipelineResourceStorage)
        : 
        mScene{ scene }, 
        mDevice{ device }, 
        mResourceProducer{ resourceProducer },
        mTopAccelerationStructure{ device, resourceProducer }, 
        mPipelineResourceStorage{ pipelineResourceStorage }
    {
        mTopAccelerationStructure.SetDebugName("All Meshes Top RT AS");
    }        

    void SceneGPUStorage::UploadMeshes()
    {
        auto& meshes = mScene->Meshes();

        mBottomAccelerationStructures.clear();

        auto& package1P1N1UV1T1BT = std::get<UploadBufferPackage<Vertex1P1N1UV1T1BT>>(mUploadBuffers);
        package1P1N1UV1T1BT.Vertices.reserve(mScene->TotalVertexCount());
        package1P1N1UV1T1BT.Indices.reserve(mScene->TotalIndexCount());

        for (Mesh& mesh : meshes)
        {
            assert_format(!mesh.Vertices().empty(), "Empty meshes are not allowed");

            VertexStorageLocation locationInStorage = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
                mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());

            mesh.SetVertexStorageLocation(locationInStorage);
        }

        auto quadVertices = fplus::transform([](const glm::vec3& p) { return Vertex1P1N1UV1T1BT{ glm::vec4{p, 1.0f} }; }, DrawablePrimitive::UnitQuadVertices);

        mUnitQuadVertexLocation = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
            quadVertices.data(), quadVertices.size(),
            DrawablePrimitive::UnitQuadIndices.data(), DrawablePrimitive::UnitQuadIndices.size());

        mUnitCubeVertexLocation = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
            mScene->UnitCube().Vertices().data(), mScene->UnitCube().Vertices().size(), 
            mScene->UnitCube().Indices().data(), mScene->UnitCube().Indices().size());

        mUnitSphereVertexLocation = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
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

        auto getSamplerIndex = [this](Material::WrapMode wrapMode) -> uint32_t
        {
            switch (wrapMode)
            {
            case Material::WrapMode::Clamp: return mPipelineResourceStorage->GetSamplerDescriptor(SamplerNames::AnisotropicClamp)->IndexInHeapRange();
            case Material::WrapMode::Mirror: return mPipelineResourceStorage->GetSamplerDescriptor(SamplerNames::AnisotropicMirror)->IndexInHeapRange();
            case Material::WrapMode::Repeat: return mPipelineResourceStorage->GetSamplerDescriptor(SamplerNames::AnisotropicWrap)->IndexInHeapRange();
            default: return mPipelineResourceStorage->GetSamplerDescriptor(SamplerNames::AnisotropicClamp)->IndexInHeapRange();
            }
        };

        for (Material& material : materials)
        {
            // All ltc look-up tables are expected to be of the same size
            auto lut0SpecularSize = material.LTC_LUT_MatrixInverse_Specular->HALTexture()->Dimensions();

            GPUMaterialTableEntry materialEntry{
                material.DiffuseAlbedoMap.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.NormalMap.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.RoughnessMap.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.MetalnessMap.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.AOMap.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.DisplacementMap.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.DistanceField.Texture->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_MatrixInverse_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Matrix_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Terms_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_MatrixInverse_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Matrix_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_Terms_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                lut0SpecularSize.Width,
                // Right now we use wrap mode from diffuse albedo and apply it for all textures in material, which should be sufficient.
                getSamplerIndex(material.DiffuseAlbedoMap.Wrapping) 
            };

            material.GPUMaterialTableIndex = materialIndex;

            mMaterialTable->Write(&materialEntry, materialIndex, 1);
            ++materialIndex;
        }
    }

    void SceneGPUStorage::UploadInstances()
    {
        mTopAccelerationStructure.Clear();
        UploadMeshInstances();
        UploadLights();
        UploadDebugGIProbes();
        mTopAccelerationStructure.Build();
        mScene->MapEntitiesToGPUIndices();
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

        auto uploadInstance = [&](const GPUMeshInstanceTableEntry& gpuInstance, const VertexStorageLocation& vertexLocations, auto&& sceneObject)
        {
            sceneObject.SetIndexInGPUTable(instanceIdx);

            BottomRTAS& blas = mBottomAccelerationStructures[vertexLocations.BottomAccelerationStructureIndex];

            HAL::RayTracingTopAccelerationStructure::InstanceInfo instanceInfo{
                instanceIdx, std::underlying_type_t<GPUInstanceMask>(GPUInstanceMask::Mesh), std::underlying_type_t<GPUInstanceHitGroupContribution>(GPUInstanceHitGroupContribution::Mesh)
            };

            mTopAccelerationStructure.AddInstance(blas, instanceInfo, gpuInstance.InstanceWorldMatrix);
            mMeshInstanceTable->Write(&gpuInstance, instanceIdx, 1);

            ++instanceIdx;
        };

        for (MeshInstance& instance : meshInstances)
        {
            GPUMeshInstanceTableEntry instanceEntry{
                glm::mat4{1.0f},
                glm::mat4{1.0f},
                glm::mat4{1.0f},
        /*        instance.Transformation().ModelMatrix(),
                instance.PrevTransformation().ModelMatrix(),
                instance.Transformation().NormalMatrix(),*/
                instance.AssociatedMaterial()->GPUMaterialTableIndex,
                instance.AssociatedMesh()->LocationInVertexStorage().VertexBufferOffset,
                instance.AssociatedMesh()->LocationInVertexStorage().IndexBufferOffset,
                instance.AssociatedMesh()->LocationInVertexStorage().IndexCount,
                instance.AssociatedMesh()->HasTangentSpace()
            };

            uploadInstance(instanceEntry, instance.AssociatedMesh()->LocationInVertexStorage(), instance);
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

                light.SetIndexInGPUTable(index);
                light.SetVertexStorageLocation(vertexLocation);

                HAL::RayTracingTopAccelerationStructure::InstanceInfo instanceInfo{
                    index, std::underlying_type_t<GPUInstanceMask>(GPUInstanceMask::Light), std::underlying_type_t<GPUInstanceHitGroupContribution>(GPUInstanceHitGroupContribution::Light)
                };

                BottomRTAS& blas = mBottomAccelerationStructures[vertexLocation.BottomAccelerationStructureIndex];
                mTopAccelerationStructure.AddInstance(blas, instanceInfo, light.ModelMatrix());

                ++index;
                ++lightCount;
                ++mLightTablePartitionInfo.TotalLightsCount;
            }
        };

        uploadLights(mScene->SphericalLights(), mLightTablePartitionInfo.SphericalLightsOffset, mLightTablePartitionInfo.SphericalLightsCount, mUnitSphereVertexLocation);
        uploadLights(mScene->RectangularLights(), mLightTablePartitionInfo.RectangularLightsOffset, mLightTablePartitionInfo.RectangularLightsCount, mUnitQuadVertexLocation);
        uploadLights(mScene->DiskLights(), mLightTablePartitionInfo.EllipticalLightsOffset, mLightTablePartitionInfo.EllipticalLightsCount, mUnitQuadVertexLocation);
    }

    void SceneGPUStorage::UploadDebugGIProbes()
    {
        if (!mScene->GlobalIlluminationManager().GIDebugEnabled)
            return;

        // We upload probe spheres for debug probe mouse picking 
        const IrradianceField& L = mScene->GlobalIlluminationManager().ProbeField;

        for (uint32_t probeIdx = 0; probeIdx < L.GetTotalProbeCount(); ++probeIdx)
        {
            glm::vec3 probePosition = L.GetProbePosition(probeIdx);

            HAL::RayTracingTopAccelerationStructure::InstanceInfo instanceInfo{
                probeIdx,
                std::underlying_type_t<GPUInstanceMask>(GPUInstanceMask::DebugGIProbe),
                std::underlying_type_t<GPUInstanceHitGroupContribution>(GPUInstanceHitGroupContribution::DebugGIProbe)
            };

            Geometry::Transformation probeTransform{ glm::vec3{L.DebugProbeRadius() * 2}, probePosition, glm::quat{} };

            BottomRTAS& blas = mBottomAccelerationStructures[mUnitSphereVertexLocation.BottomAccelerationStructureIndex];
            mTopAccelerationStructure.AddInstance(blas, instanceInfo, probeTransform.ModelMatrix());
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
        gpuCamera.FoVH = glm::radians(camera.FOVH());
        gpuCamera.FoVV = glm::radians(camera.FOVV());
        gpuCamera.FoVHTan = tan(gpuCamera.FoVH);
        gpuCamera.FoVVTan = tan(gpuCamera.FoVV);
        gpuCamera.AspectRatio = camera.AspectRatio();

        return gpuCamera;
    }

    GPUIrradianceField SceneGPUStorage::IrradianceFieldGPURepresentation() const
    {
        const IrradianceField& L = mScene->GlobalIlluminationManager().ProbeField;

        GPUIrradianceField field{};
        field.GridSize = L.GridSize();
        field.CellSize = L.CellSize();
        field.GridCornerPosition = L.CornerPosition();
        field.RaysPerProbe = L.RaysPerProbe();
        field.TotalProbeCount = L.GetTotalProbeCount();
        field.RayHitInfoTextureSize = { L.GetRayHitInfoTextureSize().Width, L.GetRayHitInfoTextureSize().Height };
        field.RayHitInfoTextureIdx = 0; // Determined in render pass
        field.ProbeRotation = L.ProbeRotation();
        field.IrradianceProbeAtlasSize = { L.GetIrradianceProbeAtlasSize().Width, L.GetIrradianceProbeAtlasSize().Height };
        field.DepthProbeAtlasSize = { L.GetDepthProbeAtlasSize().Width, L.GetDepthProbeAtlasSize().Height };
        field.IrradianceProbeAtlasProbesPerDimension = L.GetIrradianceProbeAtlasProbesPerDimension();
        field.DepthProbeAtlasProbesPerDimension = L.GetDepthProbeAtlasProbesPerDimension();
        field.IrradianceProbeSize = L.GetIrradianceProbeSize().Width;
        field.DepthProbeSize = L.GetDepthProbeSize().Width;
        field.IrradianceProbeAtlasTexIdx = 0; // Determined in render pass
        field.DepthProbeAtlasTexIdx = 0; // Determined in render pass
        field.DebugProbeRadius = L.DebugProbeRadius();
        return field;
    }

    uint32_t SceneGPUStorage::CompressedLightPartitionInfo() const
    {
        uint32_t compressed = 0;
        compressed |= (mLightTablePartitionInfo.SphericalLightsCount & 0xFF) << 24;
        compressed |= (mLightTablePartitionInfo.RectangularLightsCount & 0xFF) << 16;
        compressed |= (mLightTablePartitionInfo.EllipticalLightsCount & 0xFF) << 8;
        return compressed;
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
