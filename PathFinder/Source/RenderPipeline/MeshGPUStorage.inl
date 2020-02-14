namespace PathFinder
{

    template < template < class ... > class Container, class ... Args >
    void MeshGPUStorage::StoreMeshes(Container<Mesh, Args...>& meshes)
    {
        mBottomAccelerationStructures.clear();

        for (Mesh& mesh : meshes)
        {
            VertexStorageLocation locationInStorage = WriteToTemporaryBuffers(
                mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());

            mesh.SetVertexStorageLocation(locationInStorage);
            WriteToTemporaryBuffers(mesh.Vertices().data(), mesh.Vertices().size(), mesh.Indices().data(), mesh.Indices().size());
        }
        
        SubmitTemporaryBuffersToGPU<Vertex1P1N1UV1T1BT>();
    }

    template < template < class ... > class Container, class ... Args >
    void MeshGPUStorage::UpdateMeshInstanceTable(Container<MeshInstance, Args...>& meshInstances)
    {
        uint8_t alignment = 128;

        if (!mInstanceTable || mInstanceTable->HALBuffer()->ElementCapacity<GPUInstanceTableEntry>(alignment) < meshInstances.size())
        {
            HAL::Buffer::Properties<GPUInstanceTableEntry> props{ meshInstances.size(), alignment };
            mInstanceTable = mResourceProducer->NewBuffer(props, Memory::GPUResource::UploadStrategy::DirectAccess);
        }

        mInstanceTable->RequestWrite();
        mTopAccelerationStructure.Clear();

        GPUInstanceIndex instanceIndex = 0;

        for (MeshInstance& instance : meshInstances)
        {
            GPUInstanceTableEntry instanceEntry{
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
            instance.SetGPUInstanceIndex(instanceIndex);

            mInstanceTable->Write(&instanceEntry, instanceIndex, 1, alignment);
            ++instanceIndex;
        }

        mTopAccelerationStructure.Build();
        mTopASBarriers = {};
        mTopASBarriers.AddBarrier(mTopAccelerationStructure.UABarrier());
    }

    template <class Vertex>
    VertexStorageLocation MeshGPUStorage::WriteToTemporaryBuffers(const Vertex* vertices, uint32_t vertexCount, const uint32_t* indices, uint32_t indexCount)
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

        return location;
    }

    template <class Vertex>
    void MeshGPUStorage::SubmitTemporaryBuffersToGPU()
    {
        auto& uploadBuffers = std::get<UploadBufferPackage<Vertex>>(mUploadBuffers);
        auto& finalBuffers = std::get<FinalBufferPackage<Vertex>>(mFinalBuffers);

        if (!uploadBuffers.Vertices.empty())
        {
            HAL::Buffer::Properties<Vertex> properties{ uploadBuffers.Vertices.size() };
            finalBuffers.VertexBuffer = mResourceProducer->NewBuffer(properties);
            finalBuffers.VertexBuffer->RequestWrite();
            finalBuffers.VertexBuffer->Write(uploadBuffers.Vertices.data(), 0, uploadBuffers.Vertices.size());
            uploadBuffers.Vertices.clear();
        }

        if (!uploadBuffers.Indices.empty())
        {
            HAL::Buffer::Properties<uint32_t> properties{ uploadBuffers.Indices.size() };
            finalBuffers.IndexBuffer = mResourceProducer->NewBuffer(properties);
            finalBuffers.IndexBuffer->RequestWrite();
            finalBuffers.IndexBuffer->Write(uploadBuffers.Vertices.data(), 0, uploadBuffers.Vertices.size());
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
            blas.SetDebugName("Mesh BottomAS");

            blas.Build();
            mBottomASBarriers.AddBarrier(blas.UABarrier());
        }

        uploadBuffers.Locations.clear();
    }

}

