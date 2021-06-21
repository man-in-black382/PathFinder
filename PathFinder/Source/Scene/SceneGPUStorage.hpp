#pragma once

#include <HardwareAbstractionLayer/Device.hpp>
#include <HardwareAbstractionLayer/CommandQueue.hpp>
#include <HardwareAbstractionLayer/Buffer.hpp>
#include <HardwareAbstractionLayer/ResourceBarrier.hpp>

#include <Memory/GPUResourceProducer.hpp>

#include "Mesh.hpp"
#include "MeshInstance.hpp"
#include "Vertices/Vertex1P1N1UV1T1BT.hpp"
#include "Vertices/Vertex1P1N1UV.hpp"
#include "Vertices/Vertex1P3.hpp"
#include "FlatLight.hpp"
#include "SphericalLight.hpp"
#include "VertexStorageLocation.hpp"
#include "Sky.hpp"
#include "SceneGPUTypes.hpp"

#include <RenderPipeline/BottomRTAS.hpp>
#include <RenderPipeline/TopRTAS.hpp>
#include <RenderPipeline/PipelineResourceStorage.hpp>

#include <vector>
#include <memory>
#include <tuple>

namespace PathFinder
{
    class Scene;
    struct RenderSettings;

    class SceneGPUStorage
    {
    public:
        SceneGPUStorage(
            Scene* scene, 
            const HAL::Device* device,
            Memory::GPUResourceProducer* resourceProducer, 
            const PipelineResourceStorage* pipelineResourceStorage,
            const RenderSurfaceDescription* renderSurfaceDescription,
            const RenderSettings* renderSettings);

        void UploadMeshes();
        void UploadMaterials();
        void UploadInstances();

        GPUCamera GetCameraGPURepresentation();
        std::array<ArHosekSkyModelStateGPU, 3> GetSkyGPURepresentation() const;
        GPUIlluminanceField GetIlluminanceFieldGPURepresentation() const;
        uint32_t GetCompressedLightPartitionInfo() const;

    private:
        template <class Vertex>
        struct UploadBufferPackage
        {
            std::vector<Vertex> Vertices;
            std::vector<uint32_t> Indices;
            std::vector<VertexStorageLocation> Locations;
        };

        template <class Vertex>
        struct FinalBufferPackage
        {
            Memory::GPUResourceProducer::BufferPtr VertexBuffer;
            Memory::GPUResourceProducer::BufferPtr IndexBuffer;
        };

        template <class Vertex>
        void SubmitTemporaryBuffersToGPU();

        void UploadMeshInstances();
        void UploadLights();
        void UploadDebugGIProbes();

        GPULightTableEntry CreateLightGPUTableEntry(const FlatLight& light) const;
        GPULightTableEntry CreateLightGPUTableEntry(const SphericalLight& light) const;
        GPULightTableEntry CreateSunGPUTableEntry(const Sky& sky) const;

        template <class Vertex>
        VertexStorageLocation WriteToTemporaryBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices = nullptr, uint32_t indexCount = 0);

        std::tuple<UploadBufferPackage<Vertex1P1N1UV1T1BT>, UploadBufferPackage<Vertex1P1N1UV>, UploadBufferPackage<Vertex1P3>> mUploadBuffers;
        std::tuple<FinalBufferPackage<Vertex1P1N1UV1T1BT>, FinalBufferPackage<Vertex1P1N1UV>, FinalBufferPackage<Vertex1P3>> mFinalBuffers;

        std::vector<BottomRTAS> mBottomAccelerationStructures;
        TopRTAS mTopAccelerationStructure;

        Memory::GPUResourceProducer::BufferPtr mMeshInstanceTable;
        Memory::GPUResourceProducer::BufferPtr mLightTable;
        Memory::GPUResourceProducer::BufferPtr mMaterialTable;

        VertexStorageLocation mUnitQuadVertexLocation;
        VertexStorageLocation mUnitCubeVertexLocation;
        VertexStorageLocation mUnitSphereVertexLocation;
        GPULightTablePartitionInfo mLightTablePartitionInfo;
        uint64_t mCameraJitterFrameIndex = 0;

        Scene* mScene;
        const HAL::Device* mDevice;
        Memory::GPUResourceProducer* mResourceProducer;
        const PipelineResourceStorage* mPipelineResourceStorage;
        const RenderSurfaceDescription* mRenderSurfaceDescription;
        const RenderSettings* mRenderSettings;

    public:
        inline const auto UnifiedVertexBuffer() const { return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).VertexBuffer.get(); }
        inline const auto UnifiedIndexBuffer() const { return std::get<FinalBufferPackage<Vertex1P1N1UV1T1BT>>(mFinalBuffers).IndexBuffer.get(); }
        inline const auto MeshInstanceTable() const { return mMeshInstanceTable.get(); }
        inline const auto LightTable() const { return mLightTable.get(); }
        inline const auto MaterialTable() const { return mMaterialTable.get(); }
        inline const auto& LightTablePartitionInfo() const { return mLightTablePartitionInfo; }
        inline const auto& TopAccelerationStructure() const { return mTopAccelerationStructure; }
        inline const auto& BottomAccelerationStructures() const { return mBottomAccelerationStructures; }
    };

}

#include "SceneGPUStorage.inl"
