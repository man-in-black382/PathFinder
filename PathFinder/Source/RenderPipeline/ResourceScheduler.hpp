#pragma once

#include "PipelineResourceStorage.hpp"
#include "RenderPassUtilityProvider.hpp"
#include "RenderPassGraph.hpp"

#include "../Foundation/BitwiseEnum.hpp"

#include <vector>

namespace PathFinder
{

    class ResourceScheduler
    {
    public:
        using MipList = std::vector<uint32_t>;

        enum class BufferReadContext
        {
            Constant, ShaderResource
        };

        enum class Flags : uint32_t
        {
            None = 0, 
            CrossFrameRead = 1 << 0, // Resource will be read across frames so it cannot participate in memory aliasing
            WillNotWrite = 1 << 1    // Resource will not be written and should not be added to the graph as write dependency
        };

        struct NewTextureProperties
        {
            NewTextureProperties(
                std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt,
                std::optional<HAL::TextureKind> kind = std::nullopt,
                std::optional<Geometry::Dimensions> dimensions = std::nullopt,
                std::optional<HAL::TypelessColorFormat> typelessFormat = std::nullopt,
                std::optional<HAL::ColorClearValue> clearValues = std::nullopt,
                uint8_t mipCount = 1,
                Flags readFlags = Flags::None)
                : 
                TypelessFormat{ typelessFormat }, ShaderVisibleFormat{ shaderVisibleFormat }, 
                Kind{ kind }, Dimensions{ dimensions }, ClearValues{ clearValues }, MipCount{ mipCount }, Flags{ readFlags } {}

            std::optional<HAL::TypelessColorFormat> TypelessFormat;
            std::optional<HAL::ColorFormat> ShaderVisibleFormat;
            std::optional<HAL::TextureKind> Kind;
            std::optional<Geometry::Dimensions> Dimensions;
            std::optional<HAL::ColorClearValue> ClearValues;
            uint8_t MipCount;
            Flags Flags;
        };

        struct NewDepthStencilProperties
        {
            NewDepthStencilProperties(
                std::optional<HAL::DepthStencilFormat> format = std::nullopt,
                std::optional<Geometry::Dimensions> dimensions = std::nullopt,
                uint8_t mipCount = 1,
                Flags readFlags = Flags::None)
                : 
                Format{ format }, Dimensions{ dimensions }, MipCount{ mipCount }, Flags{ readFlags } {}

            std::optional<HAL::DepthStencilFormat> Format;
            std::optional<Geometry::Dimensions> Dimensions;
            uint8_t MipCount;
            Flags Flags;
        };

        template <class T>
        struct NewBufferProperties
        {
            NewBufferProperties(uint64_t capacity = 1, uint64_t perElementAlignment = 1, Flags readFlags = Flags::None)
                : Capacity{ capacity }, PerElementAlignment{ perElementAlignment }, Flags{ readFlags } {}

            uint64_t Capacity;
            uint64_t PerElementAlignment;
            Flags Flags;
        };

        struct NewByteBufferProperties : public NewBufferProperties<uint8_t> {};

        ResourceScheduler(PipelineResourceStorage* manager, RenderPassUtilityProvider* utilityProvider, RenderPassGraph* passGraph);

        // Allocates new render target texture (Write Only)
        void NewRenderTarget(Foundation::Name resourceName, std::optional<NewTextureProperties> properties = std::nullopt);

        // Allocates new depth-stencil texture (Write Only)
        void NewDepthStencil(Foundation::Name resourceName, std::optional<NewDepthStencilProperties> properties = std::nullopt); 

        // Allocates new texture to be accessed as Unordered Access resource (Write)
        void NewTexture(Foundation::Name resourceName, std::optional<NewTextureProperties> properties = std::nullopt); 

        // Indicates that a previously created texture will be used as a render target in the scheduling pass (Write Only)
        void UseRenderTarget(Foundation::Name resourceName, const MipList& mips = { 0 }, std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Indicates that a previously created texture will be used as a depth-stencil attachment in the scheduling pass (Write Only)
        void UseDepthStencil(Foundation::Name resourceName);

        // Read any previously created texture as a Shader Resource (Read Only)
        void ReadTexture(Foundation::Name resourceName, const MipList& mips = { 0 }, std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Access a previously created texture as an Unordered Access resource (Write)
        void WriteTexture(Foundation::Name resourceName, const MipList& mips = { 0 }, std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Allocates new buffer to be accessed as Unordered Access resource (Write Only)
        template <class T> 
        void NewBuffer(Foundation::Name resourceName, const NewBufferProperties<T>& bufferProperties = NewByteBufferProperties{ 1, 1 });

        // Read any previously created buffer either as Constant or Structured buffer (Read Only)
        void ReadBuffer(Foundation::Name resourceName, BufferReadContext readContext);

        // Access a previously created buffer as an Unordered Access Structured buffer (Write)
        void WriteBuffer(Foundation::Name resourceName);

        // Explicitly set a queue to execute render pass on
        void ExecuteOnQueue(RenderPassExecutionQueue queue);

        // Indicate that pass will use Ray Tracing Acceleration structures.
        // BVH builds will be synchronized to first pass in the graph that requests their usage.
        void UseRayTracing();

        // To be called by the engine, not render passes
        void SetCurrentlySchedulingPassNode(RenderPassGraph::Node* node);

    private:
        NewTextureProperties FillMissingFields(std::optional<NewTextureProperties> properties);
        NewDepthStencilProperties FillMissingFields(std::optional<NewDepthStencilProperties> properties);

        template <class Lambda>
        void FillCurrentPassInfo(const PipelineResourceStorageResource* resourceData, const MipList& mipList, const Lambda& lambda);

        RenderPassGraph::Node* mCurrentlySchedulingPassNode = nullptr;
        PipelineResourceStorage* mResourceStorage = nullptr;
        RenderPassUtilityProvider* mUtilityProvider = nullptr;
        RenderPassGraph* mRenderPassGraph = nullptr;

    public:
        inline const RenderSurfaceDescription& DefaultRenderSurfaceDesc() const { return mUtilityProvider->DefaultRenderSurfaceDescription; }
        inline auto FrameNumber() const { return mUtilityProvider->FrameNumber; }
    };

}

ENABLE_BITMASK_OPERATORS(PathFinder::ResourceScheduler::Flags);

#include "ResourceScheduler.inl"