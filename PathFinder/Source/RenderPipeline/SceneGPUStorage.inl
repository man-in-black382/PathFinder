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
    void SceneGPUStorage::UploadMaterials(Container<Material, Args...>& materials)
    {
        if (!mMaterialTable || mMaterialTable->ElementCapacity<GPUMaterialTableEntry>() < materials.size())
        {
            HAL::Buffer::Properties<GPUMaterialTableEntry> props{ materials.size() };
            mMaterialTable = mResourceProducer->NewBuffer(props);
            mMaterialTable->SetDebugName("Material Table");
        }

        mMaterialTable->RequestWrite();

        uint64_t materialIndex = 0;

        for (Material& material : materials)
        {
            auto lut0SpecularSize = material.LTC_LUT_0_Specular->HALTexture()->Dimensions();
            auto lut1SpecularSize = material.LTC_LUT_1_Specular->HALTexture()->Dimensions();

            assert_format(lut0SpecularSize.Width == lut0SpecularSize.Height &&
                lut0SpecularSize.Width == lut1SpecularSize.Height &&
                lut0SpecularSize.Width == lut1SpecularSize.Width,
                "LUTs should be square and equal size");

            GPUMaterialTableEntry materialEntry{
                material.AlbedoMap->GetSRDescriptor()->IndexInHeapRange(),
                material.NormalMap->GetSRDescriptor()->IndexInHeapRange(),
                material.RoughnessMap->GetSRDescriptor()->IndexInHeapRange(),
                material.MetalnessMap->GetSRDescriptor()->IndexInHeapRange(),
                material.AOMap->GetSRDescriptor()->IndexInHeapRange(),
                material.DisplacementMap->GetSRDescriptor()->IndexInHeapRange(),
                material.DistanceField->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_0_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_1_Specular->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_0_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                material.LTC_LUT_1_Diffuse->GetSRDescriptor()->IndexInHeapRange(),
                lut0SpecularSize.Width
            };

            material.GPUMaterialTableIndex = materialIndex;

            mMaterialTable->Write(&materialEntry, materialIndex, 1);
            ++materialIndex;
        }
    }

    template < template < class ... > class Container, class ... Args >
    void SceneGPUStorage::UploadMeshInstances(Container<MeshInstance, Args...>& meshInstances)
    {
        auto requiredBufferSize = meshInstances.size() + mUploadedMeshInstances;

        if (!mMeshInstanceTable || mMeshInstanceTable->ElementCapacity<GPUMeshInstanceTableEntry>() < requiredBufferSize)
        {
            HAL::Buffer::Properties<GPUMeshInstanceTableEntry> props{ requiredBufferSize };
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
                instance.AssosiatedMaterial()->GPUMaterialTableIndex,
                instance.AssosiatedMesh()->LocationInVertexStorage().VertexBufferOffset,
                instance.AssosiatedMesh()->LocationInVertexStorage().IndexBufferOffset,
                instance.AssosiatedMesh()->LocationInVertexStorage().IndexCount
            };

            BottomRTAS& blas = mBottomAccelerationStructures[instance.AssosiatedMesh()->LocationInVertexStorage().BottomAccelerationStructureIndex];
            mTopAccelerationStructure.AddInstance(blas, mUploadedMeshInstances, instance.Transformation().ModelMatrix());
            instance.SetGPUInstanceIndex(mUploadedMeshInstances);

            mMeshInstanceTable->Write(&instanceEntry, mUploadedMeshInstances, 1);
            ++mUploadedMeshInstances;
        }

        mTopAccelerationStructure.Build();
    }

    template < template < class ... > class Container, class LightT, class ... Args >
    void SceneGPUStorage::UploadLights(Container<LightT, Args...>& lights)
    {
        auto requiredBufferSize = mUploadedLights + lights.size();

        if (!mLightTable || mLightTable->ElementCapacity<GPULightTableEntry>() < requiredBufferSize)
        {
            HAL::Buffer::Properties<GPULightTableEntry> props{ requiredBufferSize };
            mLightTable = mResourceProducer->NewBuffer(props, Memory::GPUResource::UploadStrategy::DirectAccess);
            mLightTable->SetDebugName("Lights Instance Table");
        }

        mLightTable->RequestWrite();

        for (LightT& light : lights)
        {
            GPULightTableEntry lightEntry = CreateLightGPUTableEntry(light);
            mLightTable->Write(&lightEntry, mUploadedLights, 1);
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
        }

        uploadBuffers.Locations.clear();
    }

}

