#pragma once

#include "ResourceStorage.hpp"

namespace PathFinder
{

    class ResourceScheduler
    {
    public:
      /*  struct ReadOnlyMetadata
        {
            HAL::ResourceFormat::Color ShaderVisibleFormat;
        };

        struct WriteOnlyMetadata
        {
            HAL::ResourceFormat::TypelessColor DataFormat,
            HAL::ResourceFormat::Color ShaderVisibleFormat;
        };

        struct ReadWriteMetadata
        {
            HAL::ResourceFormat::Color ShaderVisibleFormat;
        };*/

        ResourceScheduler(ResourceStorage* manager);

        template <class BufferDataT>
        void WillUseRootConstantBuffer();

        void WillRenderToRenderTarget(Foundation::Name resourceName);

        void WillRenderToDepthStencil(Foundation::Name resourceName);

        void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::Color dataFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions);

        void WillRenderToRenderTarget(
            Foundation::Name resourceName,
            HAL::ResourceFormat::TypelessColor dataFormat,
            HAL::ResourceFormat::Color shaderVisisbleFormat,
            HAL::ResourceFormat::TextureKind kind,
            const Geometry::Dimensions& dimensions);

        void WillRenderToDepthStencil(
            Foundation::Name resourceName, 
            HAL::ResourceFormat::DepthStencil dataFormat,
            const Geometry::Dimensions& dimensions);

        // New scheduling system

        void NewRenderTarget(); // 
        void NewDepthStencil(); // 
        void NewTexture(); // RWTexture..[]
        void NewBuffer(); // RWStructuredBuffer
        void UseRenderTarget(); // 
        void UseDepthStencil(); // 
        void ReadTexture(); // Texture..[]
        void ReadBuffer(); // StructuredBuffer
        void ReadWriteTexture(); // RWTexture..[]
        void ReadWriteBuffer(); // RWStructuredBuffer

        /*  void WillReadTexture();

          void WillReadBuffer();

          void WillReadWriteTexture();

          void WillReadWriteBuffer();*/

    private:
        ResourceStorage* mResourceStorage;
    };

    template <class BufferDataT>
    void ResourceScheduler::WillUseRootConstantBuffer()
    {
        mResourceStorage->AllocateRootConstantBufferIfNeeded<BufferDataT>();
    }

}
