#include "SceneGPUStorage.hpp"
#include "Scene.hpp"

#include <algorithm>
#include <iterator>
#include <Foundation/Pi.hpp>
#include <Geometry/Utils.hpp>
#include <RenderPipeline/RenderSettings.hpp>
#include <RenderPipeline/DrawablePrimitive.hpp>
#include <RenderPipeline/RenderPasses/PipelineNames.hpp>
#include <fplus/fplus.hpp>

namespace PathFinder
{

    SceneGPUStorage::SceneGPUStorage(
        Scene* scene, 
        const HAL::Device* device,
        Memory::GPUResourceProducer* resourceProducer, 
        const PipelineResourceStorage* pipelineResourceStorage,
        const RenderSurfaceDescription* renderSurfaceDescription,
        const RenderSettings* renderSettings)
        : 
        mScene{ scene }, 
        mDevice{ device }, 
        mResourceProducer{ resourceProducer },
        mTopAccelerationStructure{ device, resourceProducer }, 
        mPipelineResourceStorage{ pipelineResourceStorage },
        mRenderSurfaceDescription{ renderSurfaceDescription },
        mRenderSettings{ renderSettings }
    {
        mTopAccelerationStructure.SetDebugName("All Meshes Top RT AS");
    }        

    void SceneGPUStorage::UploadMeshes()
    {
        auto& meshes = mScene->GetMeshes();

        mBottomAccelerationStructures.clear();

        auto& package1P1N1UV1T1BT = std::get<UploadBufferPackage<Vertex1P1N1UV1T1BT>>(mUploadBuffers);
        package1P1N1UV1T1BT.Vertices.reserve(mScene->GetTotalVertexCount());
        package1P1N1UV1T1BT.Indices.reserve(mScene->GetTotalIndexCount());

        for (Mesh& mesh : meshes)
        {
            assert_format(!mesh.GetVertices().empty(), "Empty meshes are not allowed");

            VertexStorageLocation locationInStorage = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
                mesh.GetVertices().data(), mesh.GetVertices().size(), mesh.GetIndices().data(), mesh.GetIndices().size());

            mesh.SetVertexStorageLocation(locationInStorage);
        }

        auto quadVertices = fplus::transform([](const glm::vec3& p) { return Vertex1P1N1UV1T1BT{ glm::vec4{p, 1.0f} }; }, DrawablePrimitive::UnitQuadVertices);

        mUnitQuadVertexLocation = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
            quadVertices.data(), quadVertices.size(),
            DrawablePrimitive::UnitQuadIndices.data(), DrawablePrimitive::UnitQuadIndices.size());

        mUnitCubeVertexLocation = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
            mScene->GetUnitCube().GetVertices().data(), mScene->GetUnitCube().GetVertices().size(), 
            mScene->GetUnitCube().GetIndices().data(), mScene->GetUnitCube().GetIndices().size());

        mUnitSphereVertexLocation = WriteToTemporaryBuffers<Vertex1P1N1UV1T1BT>(
            mScene->GetUnitSphere().GetVertices().data(), mScene->GetUnitSphere().GetVertices().size(),
            mScene->GetUnitSphere().GetIndices().data(), mScene->GetUnitSphere().GetIndices().size());

        SubmitTemporaryBuffersToGPU<Vertex1P1N1UV1T1BT>();
    }

    void SceneGPUStorage::UploadMaterials()
    {
        auto& materials = mScene->GetMaterials();

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
                getSamplerIndex(material.DiffuseAlbedoMap.Wrapping),
                material.NormalMap.Texture->Properties().Dimensions.Width > 1,
                material.IOROverride.value_or(-1.f),
                material.DiffuseAlbedoOverride.value_or(glm::vec3{-1.f}),
                material.RoughnessOverride.value_or(-1.f),
                material.SpecularAlbedoOverride.value_or(glm::vec3{-1.f}),
                material.MetalnessOverride.value_or(-1.f),
                material.TransmissionFilter.value_or(glm::vec3{-1.f}),
                material.TranslucencyOverride.value_or(-1.f),
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
        auto& meshInstances = mScene->GetMeshInstances();
        auto& sphericalLights = mScene->GetSphericalLights();
        auto& rectangularLights = mScene->GetRectangularLights();
        auto& diskLights = mScene->GetDiskLights();

        auto requiredBufferSize = meshInstances.size() + mScene->GetTotalLightCount();

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
                instance.GetTransformation().GetMatrix(),
                instance.GetPreviousTransformation().GetMatrix(),
                instance.GetTransformation().GetNormalMatrix(),
                instance.GetAssociatedMaterial()->GPUMaterialTableIndex,
                instance.GetAssociatedMesh()->GetLocationInVertexStorage().VertexBufferOffset,
                instance.GetAssociatedMesh()->GetLocationInVertexStorage().IndexBufferOffset,
                instance.GetAssociatedMesh()->GetLocationInVertexStorage().IndexCount,
                instance.GetAssociatedMesh()->HasTangentSpace(),
                instance.IsDoubleSided()
            };

            uploadInstance(instanceEntry, instance.GetAssociatedMesh()->GetLocationInVertexStorage(), instance);
        }
    }

    void SceneGPUStorage::UploadLights()
    {
        auto& sphericalLights = mScene->GetSphericalLights();
        auto& rectangularLights = mScene->GetRectangularLights();
        auto& diskLights = mScene->GetDiskLights();

        auto requiredBufferSize = mScene->GetTotalLightCount();

        if (!mLightTable || mLightTable->Capacity<GPULightTableEntry>() < requiredBufferSize)
        {
            auto properties = HAL::BufferProperties::Create<GPULightTableEntry>(requiredBufferSize);
            mLightTable = mResourceProducer->NewBuffer(properties, Memory::GPUResource::AccessStrategy::DirectUpload);
            mLightTable->SetDebugName("Lights Instance Table");
        }

        mLightTable->RequestWrite();

        // Sun goes first
        GPULightTableEntry sunEntry = CreateSunGPUTableEntry(mScene->GetSky());
        mLightTable->Write(&sunEntry, 0, 1);

        // Then local lights
        uint32_t index = 1;
        mLightTablePartitionInfo = {};
        mLightTablePartitionInfo.TotalLightsCount = 1;
        mLightTablePartitionInfo.SphericalLightsOffset = index;

        auto uploadLights = [this, &index](auto&& lights, uint32_t& tableOffset, uint32_t& lightCount, const VertexStorageLocation& vertexLocation)
        {
            tableOffset = index;

            for (auto& light : lights)
            {
                if (light.GetLuminousPower() <= 0.0)
                    continue;

                light.SetIndexInGPUTable(index);
                light.SetVertexStorageLocation(vertexLocation);
                light.ConstructModelMatrix();

                GPULightTableEntry lightEntry = CreateLightGPUTableEntry(light);
                mLightTable->Write(&lightEntry, index, 1);

                HAL::RayTracingTopAccelerationStructure::InstanceInfo instanceInfo{
                    index, std::underlying_type_t<GPUInstanceMask>(GPUInstanceMask::Light), std::underlying_type_t<GPUInstanceHitGroupContribution>(GPUInstanceHitGroupContribution::Light)
                };

                BottomRTAS& blas = mBottomAccelerationStructures[vertexLocation.BottomAccelerationStructureIndex];
                mTopAccelerationStructure.AddInstance(blas, instanceInfo, light.GetModelMatrix());

                ++index;
                ++lightCount;
                ++mLightTablePartitionInfo.TotalLightsCount;
            }
        };

        uploadLights(mScene->GetSphericalLights(), mLightTablePartitionInfo.SphericalLightsOffset, mLightTablePartitionInfo.SphericalLightsCount, mUnitSphereVertexLocation);
        uploadLights(mScene->GetRectangularLights(), mLightTablePartitionInfo.RectangularLightsOffset, mLightTablePartitionInfo.RectangularLightsCount, mUnitQuadVertexLocation);
        uploadLights(mScene->GetDiskLights(), mLightTablePartitionInfo.EllipticalLightsOffset, mLightTablePartitionInfo.EllipticalLightsCount, mUnitQuadVertexLocation);
    }

    void SceneGPUStorage::UploadDebugGIProbes()
    {
        if (!mScene->GetGIManager().GIDebugEnabled)
            return;

        // We upload probe spheres for debug probe mouse picking 
        const IlluminanceField& L = mScene->GetGIManager().ProbeField;

        for (uint32_t probeIdx = 0; probeIdx < L.GetTotalProbeCount(); ++probeIdx)
        {
            glm::vec3 probePosition = L.GetProbePosition(probeIdx);

            HAL::RayTracingTopAccelerationStructure::InstanceInfo instanceInfo{
                probeIdx,
                std::underlying_type_t<GPUInstanceMask>(GPUInstanceMask::DebugGIProbe),
                std::underlying_type_t<GPUInstanceHitGroupContribution>(GPUInstanceHitGroupContribution::DebugGIProbe)
            };

            Geometry::Transformation probeTransform{ glm::vec3{L.GetDebugProbeRadius() * 2}, probePosition, glm::quat{} };

            BottomRTAS& blas = mBottomAccelerationStructures[mUnitSphereVertexLocation.BottomAccelerationStructureIndex];
            mTopAccelerationStructure.AddInstance(blas, instanceInfo, probeTransform.GetMatrix());
        }
    }

    GPUCamera SceneGPUStorage::GetCameraGPURepresentation()
    {
        const PathFinder::Camera& camera = mScene->GetMainCamera();

        GPUCamera gpuCamera{};

        Camera::Jitter jitter = camera.GetJitter(
            mCameraJitterFrameIndex,
            mRenderSettings->TAASampleCount,
            glm::uvec2{ mRenderSurfaceDescription->Dimensions().Width, mRenderSurfaceDescription->Dimensions().Height }
        );

        gpuCamera.Position = glm::vec4{ camera.GetPosition(), 1.0 };
        gpuCamera.View = camera.GetView();
        gpuCamera.Projection = camera.GetProjection();
        gpuCamera.ViewProjection = camera.GetViewProjection();
        gpuCamera.InverseView = camera.GetInverseView();
        gpuCamera.InverseProjection = camera.GetInverseProjection();
        gpuCamera.InverseViewProjection = camera.GetInverseViewProjection();
        gpuCamera.Jitter = mRenderSettings->IsTAAEnabled ? jitter.JitterMatrix : glm::mat4{ 1.f };
        gpuCamera.UVJitter = mRenderSettings->IsTAAEnabled ? jitter.UVJitter : glm::vec2{ 0.0f };
        gpuCamera.ViewProjectionJitter = gpuCamera.Jitter * gpuCamera.ViewProjection;
        gpuCamera.ExposureValue100 = camera.GetExposureValue100();
        gpuCamera.FarPlane = camera.GetFarClipPlane();
        gpuCamera.NearPlane = camera.GetNearClipPlane();
        gpuCamera.FoVH = glm::radians(camera.GetFOVH());
        gpuCamera.FoVV = glm::radians(camera.GetFOVV());
        gpuCamera.FoVHTan = tan(gpuCamera.FoVH);
        gpuCamera.FoVVTan = tan(gpuCamera.FoVV);
        gpuCamera.AspectRatio = camera.GetAspectRatio();
        gpuCamera.Front = glm::vec4{ camera.GetFront(), 0.0 };

        ++mCameraJitterFrameIndex;

        return gpuCamera;
    }

    std::array<ArHosekSkyModelStateGPU, 3> SceneGPUStorage::GetSkyGPURepresentation() const
    {
        const ArHosekSkyModelState* skyStateR = mScene->GetSky().GetSkyModelStateR();
        const ArHosekSkyModelState* skyStateG = mScene->GetSky().GetSkyModelStateG();
        const ArHosekSkyModelState* skyStateB = mScene->GetSky().GetSkyModelStateB();
        std::array<ArHosekSkyModelStateGPU, 3> gpuStates{};

        auto copyState = [](ArHosekSkyModelStateGPU& gpuState, const ArHosekSkyModelState* cpuState)
        {
            for (auto i = 0; i < 3; ++i)
            {
                for (auto configIdx = 0; configIdx < 9; ++configIdx)
                {
                    gpuState.Configs[i][configIdx] = cpuState->configs[i][configIdx];
                }

                gpuState.Radiances[i] = cpuState->radiances[i];
            }
        };

        copyState(gpuStates[0], skyStateR);
        copyState(gpuStates[1], skyStateG);
        copyState(gpuStates[2], skyStateB);

        return gpuStates;
    }

    GPUIlluminanceField SceneGPUStorage::GetIlluminanceFieldGPURepresentation() const
    {
        const IlluminanceField& L = mScene->GetGIManager().ProbeField;

        GPUIlluminanceField field{};
        field.GridSize = L.GetGridSize();
        field.CellSize = L.GetCellSize();
        field.GridCornerPosition = L.GetCornerPosition();
        field.RaysPerProbe = L.GetRaysPerProbe();
        field.TotalProbeCount = L.GetTotalProbeCount();
        field.RayHitInfoTextureSize = { L.GetRayHitInfoTextureSize().Width, L.GetRayHitInfoTextureSize().Height };
        field.RayHitInfoTextureIdx = 0; // Determined in render pass
        field.ProbeRotation = L.GetProbeRotation();
        field.IlluminanceProbeAtlasSize = { L.GetIlluminanceProbeAtlasSize().Width, L.GetIlluminanceProbeAtlasSize().Height };
        field.DepthProbeAtlasSize = { L.GetDepthProbeAtlasSize().Width, L.GetDepthProbeAtlasSize().Height };
        field.IlluminanceProbeAtlasProbesPerDimension = L.GetIlluminanceProbeAtlasProbesPerDimension();
        field.DepthProbeAtlasProbesPerDimension = L.GetDepthProbeAtlasProbesPerDimension();
        field.IlluminanceProbeSize = L.GetIlluminanceProbeSize().Width;
        field.DepthProbeSize = L.GetDepthProbeSize().Width;
        field.CurrentIlluminanceProbeAtlasTexIdx = 0; // Determined in render pass
        field.CurrentDepthProbeAtlasTexIdx = 0; // Determined in render pass
        field.DebugProbeRadius = L.GetDebugProbeRadius();
        field.SpawnedProbePlanesCount = L.GetSpawnedProbePlanesCount();
        field.IlluminanceHysteresisDecrease = L.GetIlluminanceHysteresisDecrease();
        field.DepthHysteresisDecrease = L.GetDepthHysteresisDecrease();
        return field;
    }

    uint32_t SceneGPUStorage::GetCompressedLightPartitionInfo() const
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

        switch (light.GetLightType())
        {
        case FlatLight::Type::Disk: lightType = GPULightTableEntry::LightType::Disk; break;
        case FlatLight::Type::Rectangle: lightType = GPULightTableEntry::LightType::Rectangle; break;
        }

        return{
                glm::vec4(light.GetPosition(), 1.0f),
                glm::vec4(light.GetColor().R(), light.GetColor().G(), light.GetColor().B(), 0.0f),
                light.GetLuminance(),
                light.GetWidth(),
                light.GetHeight(),
                std::underlying_type_t<GPULightTableEntry::LightType>(lightType),
                light.GetModelMatrix(),
                glm::mat4_cast(light.GetRotation()),
                mUnitQuadVertexLocation.VertexBufferOffset,
                mUnitQuadVertexLocation.IndexBufferOffset,
                mUnitQuadVertexLocation.IndexCount
        };
    }

    GPULightTableEntry SceneGPUStorage::CreateLightGPUTableEntry(const SphericalLight& light) const
    {
        return{
                glm::vec4(light.GetPosition(), 1.0f),
                glm::vec4(light.GetColor().R(), light.GetColor().G(), light.GetColor().B(), 0.0f),
                light.GetLuminance(),
                light.GetRadius(),
                light.GetRadius(),
                std::underlying_type_t<GPULightTableEntry::LightType>(GPULightTableEntry::LightType::Sphere),
                light.GetModelMatrix(),
                glm::mat4{1.0f},
                mUnitSphereVertexLocation.VertexBufferOffset,
                mUnitSphereVertexLocation.IndexBufferOffset,
                mUnitSphereVertexLocation.IndexCount
        };
    }

    GPULightTableEntry SceneGPUStorage::CreateSunGPUTableEntry(const Sky& sky) const
    {
        float sunDiskArea = Sky::SunDiskRadius * Sky::SunDiskRadius * M_PI;
        float sunDiskRadiusSqrt = std::sqrt(Sky::SunDiskRadius);

        return{
                glm::vec4{0.0},
                glm::vec4{0.0},
                0.0f, 0.0f, 0.0f,
                std::underlying_type_t<GPULightTableEntry::LightType>(GPULightTableEntry::LightType::Sun),
                glm::mat4{
                    glm::vec4{sky.GetSunDirection(), 0.0f},
                    glm::vec4{sky.GetSolarIlluminance(), 0.0f},
                    glm::vec4{sky.GetSolarLuminance(), 0.0f},
                    glm::vec4{Sky::SunDiskRadius, Sky::SunSolidAngle, sunDiskArea, sunDiskRadiusSqrt}
                },
                glm::mat4{Geometry::OrientationMatrix(sky.GetSunDirection())},
                0, 0, 0
        };
    }

}
