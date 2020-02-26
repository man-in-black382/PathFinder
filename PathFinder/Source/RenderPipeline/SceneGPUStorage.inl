namespace PathFinder
{

    template < template < class ... > class Container, class ... Args >
    void SceneGPUStorage::UploadMeshes(Container<Mesh, Args...>& meshes)
    {
        mBottomAccelerationStructures.clear();

        for (Mesh& mesh : meshes)
        {
            VertexStorageLocation locationInStorage = WriteToTemporaryBuffers(
                mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());

            mesh.SetVertexStorageLocation(locationInStorage);
        }
        
        SubmitTemporaryBuffersToGPU<Vertex1P1N1UV1T1BT>();
    }

    template < template < class ... > class Container, class ... Args >
    void SceneGPUStorage::UploadMeshInstances(Container<MeshInstance, Args...>& meshInstances)
    {
        uint8_t alignment = 128;

        if (!mMeshInstanceTable || mMeshInstanceTable->ElementCapacity<GPUMeshInstanceTableEntry>(alignment) < meshInstances.size())
        {
            HAL::Buffer::Properties<GPUMeshInstanceTableEntry> props{ meshInstances.size(), alignment };
            mMeshInstanceTable = mResourceProducer->NewBuffer(props, Memory::GPUResource::UploadStrategy::DirectAccess);
            mMeshInstanceTable->SetDebugName("Mesh Instance Table");
        }

        mMeshInstanceTable->RequestWrite();
        mTopAccelerationStructure.Clear();

        for (MeshInstance& instance : meshInstances)
        {
            GPUMeshInstanceTableEntry instanceEntry{
                instance.Transformation().ModelMatrix(),
                instance.Transformation().NormalMatrix(),
                instance.AssosiatedMaterial()->AlbedoMap->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMaterial()->NormalMap->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMaterial()->RoughnessMap->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMaterial()->MetalnessMap->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMaterial()->AOMap->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMaterial()->DisplacementMap->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMaterial()->DistanceField->GetOrCreateSRDescriptor()->IndexInHeapRange(),
                instance.AssosiatedMesh()->LocationInVertexStorage().VertexBufferOffset,
                instance.AssosiatedMesh()->LocationInVertexStorage().IndexBufferOffset,
                instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount
            };

            BottomRTAS& blas = mBottomAccelerationStructures[instance.AssosiatedMesh()->LocationInVertexStorage().BottomAccelerationStructureIndex];
            mTopAccelerationStructure.AddInstance(blas, instanceIndex, instance.Transformation().ModelMatrix());
            instance.SetGPUInstanceIndex(mUploadedMeshInstances);

            mMeshInstanceTable->Write(&instanceEntry, instanceIndex, 1, alignment);
            ++mUploadedMeshInstances;
        }

        mTopAccelerationStructure.Build();
        mTopASBarriers = {};
        mTopASBarriers.AddBarrier(mTopAccelerationStructure.UABarrier());
    }

    template < template < class ... > class Container, class LightT, class ... Args >
    void UploadLights(Container<LightT, Args...>& lights)
    {
        uint8_t alignment = 128;

        if (!mLightInstanceTable || mLightInstanceTable->ElementCapacity<GPULightInstanceTableEntry>(alignment) < lights.size())
        {
            HAL::Buffer::Properties<GPUMeshInstanceTableEntry> props{ lights.size(), alignment };
            mLightInstanceTable = mResourceProducer->NewBuffer(props, Memory::GPUResource::UploadStrategy::DirectAccess);
            mLightInstanceTable->SetDebugName("Lights Instance Table");
        }

        mLightInstanceTable->RequestWrite();

        for (LightT& light : lights)
        {
            GPULightInstanceTableEntry lightEntry = CreateLightGPUTableEntry(light);
            mLightInstanceTable->Write(&lightEntry, instanceIndex, 1, alignment);
            light.SetGPULightTableIndex(mUploadedLights);
            ++mUploadedLights;
        }
    }

    template <class Vertex>
    VertexStorageLocation SceneGPUStorage::WriteToTemporaryBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
    {
        auto& package = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto vertexStartIndex = package.Vertices.size();
        auto indexStartIndex = package.Indices.size();

        auto blasIndex = mBottomAccelerationStructures.size();

        VertexStorageLocation location{ vertexStartIndex, vertexCount, indexStartIndex, indexCount, blasIndex };

        package.Vertices.reserve(package.Vertices.size() + vertexCount);
        package.Indices.reserve(package.Indices.size() + indexCount);
        package.Locations.push_back(location);

        std::copy(vertices, vertices + vertexCount, std::back_inserter(package.Vertices));
        std::copy(indices, indices + indexCount, std::back_inserter(package.Indices));

        mBottomAccelerationStructures.emplace_back(mDevice, mResourceProducer);
        mBottomAccelerationStructures.back().SetDebugName("Mesh Bottom RT AS");

        return location;
    }

    template <class Vertex>
    void SceneGPUStorage::SubmitTemporaryBuffersToGPU()
    {
        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (!uploadBuffers.Vertices.empty())
        {
            HAL::Buffer::Properties<Vertex> properties{ uploadBuffers.Vertices.size() };
            finalBuffers.VertexBuffer = mResourceProducer->NewBuffer(properties);
            finalBuffers.VertexBuffer->RequestWrite();
            finalBuffers.VertexBuffer->Write(uploadBuffers.Vertices.data(), 0, uploadBuffers.Vertices.size());
            finalBuffers.VertexBuffer->SetDebugName("Unified Vertex Buffer");
            uploadBuffers.Vertices.clear();
        }

        if (!uploadBuffers.Indices.empty())
        {
            HAL::Buffer::Properties<uint32_t> properties{ uploadBuffers.Indices.size() };
            finalBuffers.IndexBuffer = mResourceProducer->NewBuffer(properties);
            finalBuffers.IndexBuffer->RequestWrite();
            finalBuffers.IndexBuffer->Write(uploadBuffers.Indices.data(), 0, uploadBuffers.Indices.size());
            finalBuffers.IndexBuffer->SetDebugName("Unified Index Buffer");
            uploadBuffers.Indices.clear();
        }

        mBottomASBarriers = {};

        for (const VertexStorageLocation& location : uploadBuffers.Locations)
        {
            BottomRTAS& blas = mBottomAccelerationStructures[location.BottomAccelerationStructureIndex];

            HAL::RayTracingGeometry blasGeometry{
                finalBuffers.VertexBuffer->HALBuffer(), location.VertexBufferOffset, location.VertexCount, sizeof(Vertex), HAL::ColorFormat::RGB32_Float,
                finalBuffers.IndexBuffer->HALBuffer(), location.IndexBufferOffset, location.IndexCount, sizeof(uint32_t), HAL::ColorFormat::R32_Unsigned,
                glm::mat4x4{}, true
            };

            blas.AddGeometry(blasGeometry);
            blas.Build();

            mBottomASBarriers.AddBarrier(blas.UABarrier());
        }

        uploadBuffers.Locations.clear();
    }

}

