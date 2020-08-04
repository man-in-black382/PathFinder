namespace PathFinder
{

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
            HAL::BufferProperties<Vertex> properties{ uploadBuffers.Vertices.size() };
            finalBuffers.VertexBuffer = mResourceProducer->NewBuffer(properties);
            finalBuffers.VertexBuffer->RequestWrite();
            finalBuffers.VertexBuffer->Write(uploadBuffers.Vertices.data(), 0, uploadBuffers.Vertices.size());
            finalBuffers.VertexBuffer->SetDebugName("Unified Vertex Buffer");
            uploadBuffers.Vertices.clear();
        }

        if (!uploadBuffers.Indices.empty())
        {
            HAL::BufferProperties<uint32_t> properties{ uploadBuffers.Indices.size() };
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

