#pragma once

#include "PipelineResourceStorage.hpp"

namespace PathFinder
{

    class ResourceScheduler
    {
    public:
        enum class BufferReadContext
        {
            Constant, ShaderResource
        };

        struct NewTextureProperties
        {
            NewTextureProperties(
                std::optional<HAL::TextureKind> kind = std::nullopt,
                std::optional<Geometry::Dimensions> dimensions = std::nullopt,
                std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt,
                std::optional<HAL::TypelessColorFormat> typelessFormat = std::nullopt,
                uint8_t mipCount = 1)
                : 
                TypelessFormat{ typelessFormat }, ShaderVisibleFormat{ shaderVisibleFormat }, 
                Kind{ kind }, Dimensions{ dimensions }, MipCount{ mipCount } {}

            std::optional<HAL::TypelessColorFormat> TypelessFormat;
            std::optional<HAL::ColorFormat> ShaderVisibleFormat;
            std::optional<HAL::TextureKind> Kind;
            std::optional<Geometry::Dimensions> Dimensions;
            std::optional<uint8_t> MipCount;
            uint64_t TextureCount = 1;
        };

        struct NewDepthStencilProperties
        {
            NewDepthStencilProperties(
                std::optional<HAL::DepthStencilFormat> format = std::nullopt,
                std::optional<Geometry::Dimensions> dimensions = std::nullopt)
                : 
                Format{ format }, Dimensions{ dimensions } {}

            std::optional<HAL::DepthStencilFormat> Format;
            std::optional<Geometry::Dimensions> Dimensions;
            uint64_t TextureCount = 1;
        };

        template <class T>
        struct NewBufferProperties
        {
            NewBufferProperties(uint64_t capacity = 1, uint64_t perElementAlignment = 1)
                : Capacity{ capacity }, PerElementAlignment{ perElementAlignment } {}

            uint64_t Capacity;
            uint64_t PerElementAlignment;
            uint64_t BufferCount = 1;
        };

        struct NewByteBufferProperties : public NewBufferProperties<uint8_t> {};

        ResourceScheduler(PipelineResourceStorage* manager, const RenderSurfaceDescription& defaultRenderSurface);

        // Allocates new render target texture (Write Only)
        void NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties = std::nullopt);

        // Allocates new depth-stencil texture (Write Only)
        void NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties = std::nullopt); 

        // Allocates new texture to be accessed as Unordered Access resource (Read/Write)
        void NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties = std::nullopt); 

        // Indicates that a previously created render target will be used as a render target in the scheduling pass (Write Only)
        void UseRenderTarget(Foundation::Name resourceName, std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Indicates that a previously created depth-stencil texture will be used as a depth-stencil attachment in the scheduling pass (Write Only)
        void UseDepthStencil(Foundation::Name resourceName); 

        // Read any previously created texture as a Shader Resource (Read Only)
        void ReadTexture(Foundation::Name resourceName, std::optional<HAL::ColorFormat> concreteFormat = std::nullopt); 

        // Access a previously created texture as an Unordered Access resource (Read/Write)
        void ReadWriteTexture(Foundation::Name resourceName, std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Allocates new buffer to be accessed as Unordered Access resource (Read/Write)
        template <class T> 
        void NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties = NewByteBufferProperties{ 1, 1 });

        // Read any previously created buffer either as Constant or Structured buffer (Read Only)
        void ReadBuffer(Foundation::Name resourceName, BufferReadContext readContext); 

        // Access a previously created buffer as an Unordered Access Structured buffer (Read/Write)
        void ReadWriteBuffer(Foundation::Name resourceName);

    private:
        NewTextureProperties FillMissingFields(std::optional<NewTextureProperties> properties);
        NewDepthStencilProperties FillMissingFields(std::optional<NewDepthStencilProperties> properties);

        void EnsureSingleSchedulingRequestForCurrentPass(ResourceName resourceName);

        PipelineResourceStorage* mResourceStorage;
        RenderSurfaceDescription mDefaultRenderSurfaceDesc;

    public:
        const RenderSurfaceDescription& DefaultRenderSurfaceDesc() const { return mDefaultRenderSurfaceDesc; }
    };

}

#include "ResourceScheduler.inl"