#pragma once

#include "PipelineResourceStorage.hpp"

namespace PathFinder
{

    class ResourceScheduler
    {
    public:
        struct NewTextureProperties
        {
            NewTextureProperties(
                std::optional<HAL::ResourceFormat::TextureKind> kind = std::nullopt,
                std::optional<Geometry::Dimensions> dimensions = std::nullopt,
                std::optional<HAL::ResourceFormat::Color> shaderVisibleFormat = std::nullopt,
                std::optional<HAL::ResourceFormat::TypelessColor> typelessFormat = std::nullopt,
                uint8_t mipCount = 0)
                : 
                TypelessFormat{ typelessFormat }, ShaderVisibleFormat{ shaderVisibleFormat }, 
                Kind{ kind }, Dimensions{ dimensions }, MipCount{ mipCount } {}

            std::optional<HAL::ResourceFormat::TypelessColor> TypelessFormat;
            std::optional<HAL::ResourceFormat::Color> ShaderVisibleFormat;
            std::optional<HAL::ResourceFormat::TextureKind> Kind;
            std::optional<Geometry::Dimensions> Dimensions;
            std::optional<uint8_t> MipCount;
        };

        struct NewDepthStencilProperties
        {
            NewDepthStencilProperties(HAL::ResourceFormat::DepthStencil format, const Geometry::Dimensions& dimensions)
                : Format{ format }, Dimensions{ dimensions } {}

            std::optional<HAL::ResourceFormat::DepthStencil> Format;
            std::optional<Geometry::Dimensions> Dimensions;
        };

        ResourceScheduler(PipelineResourceStorage* manager);

        template <class BufferDataT>
        void WillUseRootConstantBuffer();

        void NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties = std::nullopt);
        void NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties = std::nullopt); 
        void NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties = std::nullopt); 
        void NewBuffer(); // TODO: Implement buffer API (RWStructuredBuffer)
        void UseRenderTarget(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat = std::nullopt);
        void UseDepthStencil(Foundation::Name resourceName); 
        void ReadTexture(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat = std::nullopt); 
        void ReadBuffer(); // TODO: Implement buffer API (StructuredBuffer)
        void ReadWriteTexture(Foundation::Name resourceName, std::optional<HAL::ResourceFormat::Color> concreteFormat = std::nullopt);
        void ReadWriteBuffer(); // TODO: Implement buffer API (RWStructuredBuffer)

    private:
        NewTextureProperties FillMissingFields(std::optional<NewTextureProperties> properties);
        NewDepthStencilProperties FillMissingFields(std::optional<NewDepthStencilProperties> properties);

        void EnsureSingleSchedulingRequestForCurrentPass(ResourceName resourceName);

        PipelineResourceStorage* mResourceStorage;
    };

    template <class BufferDataT>
    void ResourceScheduler::WillUseRootConstantBuffer()
    {
        mResourceStorage->AllocateRootConstantBufferIfNeeded<BufferDataT>();
    }

}
