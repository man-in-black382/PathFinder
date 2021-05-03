#pragma once

#include "../PipelineResourceStorage.hpp"
#include "../PipelineSettings.hpp"
#include "../RenderPassGraph.hpp"

#include "RenderPassUtilityProvider.hpp"
#include <Foundation/BitwiseEnum.hpp>
#include <vector>

namespace PathFinder
{

    enum class BufferReadContext
    {
        Constant, AnyShader, PixelShader, NonPixelShader
    };

    enum class TextureReadContext
    {
        AnyShader, PixelShader, NonPixelShader
    };

    enum class ResourceSchedulingFlags : uint32_t
    {
        None = 0,
        CrossFrameRead = 1 << 0 // Resource will be read across frames so it cannot participate in memory aliasing
    };

    using MipList = std::vector<uint32_t>;
    using MipRange = std::pair<uint32_t, std::optional<uint32_t>>;

    struct MipSet
    {
        static MipSet Empty();
        static MipSet Explicit(const MipList& mips);
        static MipSet IndexFromStart(uint32_t index);
        static MipSet IndexFromEnd(uint32_t index);
        static MipSet FirstMip();
        static MipSet LastMip();
        static MipSet AllMips();
        static MipSet Range(uint32_t firstMip, std::optional<uint32_t> lastMip);

        using MipVariant = std::variant<MipList, MipRange, uint32_t, uint32_t>;

        std::optional<MipVariant> Combination = std::nullopt;
    };

    struct NewTextureProperties
    {
        inline static constexpr uint32_t FullMipChain = 0;

        NewTextureProperties(
            std::optional<HAL::ColorFormat> shaderVisibleFormat = std::nullopt,
            std::optional<HAL::TextureKind> kind = std::nullopt,
            std::optional<Geometry::Dimensions> dimensions = std::nullopt,
            std::optional<HAL::TypelessColorFormat> typelessFormat = std::nullopt,
            std::optional<HAL::ColorClearValue> clearValues = std::nullopt,
            std::optional<uint32_t> mipCount = std::nullopt,
            ResourceSchedulingFlags readFlags = ResourceSchedulingFlags::None)
            :
            TypelessFormat{ typelessFormat }, ShaderVisibleFormat{ shaderVisibleFormat },
            Kind{ kind }, Dimensions{ dimensions }, ClearValues{ clearValues }, MipCount{ mipCount }, Flags{ readFlags } {}

        NewTextureProperties(Foundation::Name textureNameToCopyPropertiesFrom)
            : TextureToCopyPropertiesFrom{ textureNameToCopyPropertiesFrom } {}

        std::optional<HAL::TypelessColorFormat> TypelessFormat;
        std::optional<HAL::ColorFormat> ShaderVisibleFormat;
        std::optional<HAL::TextureKind> Kind;
        std::optional<Geometry::Dimensions> Dimensions;
        std::optional<HAL::ColorClearValue> ClearValues;
        std::optional<uint32_t> MipCount;
        ResourceSchedulingFlags Flags;

        std::optional<Foundation::Name> TextureToCopyPropertiesFrom;
    };

    struct NewDepthStencilProperties
    {
        inline static constexpr uint32_t FullMipChain = 0;

        NewDepthStencilProperties(
            std::optional<HAL::DepthStencilFormat> format = std::nullopt,
            std::optional<Geometry::Dimensions> dimensions = std::nullopt,
            std::optional<uint32_t> mipCount = std::nullopt,
            ResourceSchedulingFlags readFlags = ResourceSchedulingFlags::None)
            :
            Format{ format }, Dimensions{ dimensions }, MipCount{ mipCount }, Flags{ readFlags } {}

        NewDepthStencilProperties(Foundation::Name textureNameToCopyPropertiesFrom)
            : TextureToCopyPropertiesFrom{ textureNameToCopyPropertiesFrom } {}

        std::optional<HAL::DepthStencilFormat> Format;
        std::optional<Geometry::Dimensions> Dimensions;
        std::optional<uint32_t> MipCount;
        ResourceSchedulingFlags Flags;

        std::optional<Foundation::Name> TextureToCopyPropertiesFrom;
    };

    template <class T>
    struct NewBufferProperties
    {
        NewBufferProperties(uint64_t capacity = 1, uint64_t perElementAlignment = 1, ResourceSchedulingFlags readFlags = ResourceSchedulingFlags::None)
            : Capacity{ capacity }, PerElementAlignment{ perElementAlignment }, Flags{ readFlags } {}

        NewBufferProperties(Foundation::Name bufferNameToCopyPropertiesFrom)
            : BufferToCopyPropertiesFrom{ bufferNameToCopyPropertiesFrom } {}

        uint64_t Capacity;
        uint64_t PerElementAlignment;
        ResourceSchedulingFlags Flags;

        std::optional<Foundation::Name> BufferToCopyPropertiesFrom;
    };

    struct NewByteBufferProperties : public NewBufferProperties<uint32_t> {};

    template <class ContentMediator>
    class ResourceScheduler
    {
    public:
        ResourceScheduler(PipelineResourceStorage* manager, RenderPassUtilityProvider* utilityProvider, RenderPassGraph* passGraph, const PipelineSettings* settings);

        // Allocates new render target texture (Write Only)
        void NewRenderTarget(
            Foundation::Name resourceName, 
            std::optional<NewTextureProperties> properties = std::nullopt);

        void NewRenderTarget(
            Foundation::Name resourceName,
            const MipSet& writtenMips, 
            std::optional<NewTextureProperties> properties = std::nullopt);

        // Allocates new depth-stencil texture (Write Only)
        void NewDepthStencil(
            Foundation::Name resourceName, 
            std::optional<NewDepthStencilProperties> properties = std::nullopt); 

        void NewDepthStencil(
            Foundation::Name resourceName,
            const MipSet& writtenMips,
            std::optional<NewDepthStencilProperties> properties = std::nullopt);

        // Allocates new texture to be accessed as Unordered Access resource (Write)
        void NewTexture(
            Foundation::Name resourceName, 
            std::optional<NewTextureProperties> properties = std::nullopt);

        void NewTexture(
            Foundation::Name resourceName, 
            const MipSet& writtenMips,
            std::optional<NewTextureProperties> properties = std::nullopt);

        // Indicates that a previously created texture will be used as a render target in the scheduling pass (Write Only)
        void UseRenderTarget(
            Foundation::Name resourceName,
            const MipSet& writtenMips = MipSet::AllMips(),
            std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Provide alias when writing to existing resource more then once is required
        void AliasAndUseRenderTarget(
            Foundation::Name resourceName, 
            Foundation::Name outputAliasName,
            const MipSet& writtenMips = MipSet::AllMips(),
            std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Indicates that a previously created texture will be used as a depth-stencil attachment in the scheduling pass (Write Only)
        void UseDepthStencil(Foundation::Name resourceName);

        // Provide alias when writing to existing resource more then once is required
        void AliasAndUseDepthStencil(
            Foundation::Name resourceName,
            Foundation::Name outputAliasName);

        // Read any previously created texture as a Shader Resource (Read Only)
        void ReadTexture(
            Foundation::Name resourceName,
            TextureReadContext readContext = TextureReadContext::NonPixelShader,
            const MipSet& readMips = MipSet::AllMips(),
            std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Access a previously created texture as an Unordered Access resource (Write)
        void WriteTexture(
            Foundation::Name resourceName,
            const MipSet& writtenMips = MipSet::AllMips(),
            std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Provide alias when writing to existing resource more then once is required
        void AliasAndWriteTexture(
            Foundation::Name resourceName,
            Foundation::Name outputAliasName,
            const MipSet& writtenMips = MipSet::AllMips(),
            std::optional<HAL::ColorFormat> concreteFormat = std::nullopt);

        // Allocates new buffer to be accessed as Unordered Access resource (Write Only)
        template <class T> 
        void NewBuffer(
            Foundation::Name resourceName, 
            const NewBufferProperties<T>& bufferProperties = NewByteBufferProperties{ 1, 1 });

        // Read any previously created buffer either as Constant or Structured buffer (Read Only)
        void ReadBuffer(Foundation::Name resourceName, BufferReadContext readContext);

        // Access a previously created buffer as an Unordered Access Structured buffer (Write)
        void WriteBuffer(Foundation::Name resourceName);
        // Provide alias when writing to existing resource more then once is required
        void WriteBuffer(Foundation::Name resourceName, Foundation::Name outputAliasName);

        // Indicate that pass will write to back buffer
        void WriteToBackBuffer();

        // Explicitly set a queue to execute render pass on
        void ExecuteOnQueue(RenderPassExecutionQueue queue);

        // Indicate that pass will use Ray Tracing Acceleration structures.
        // BVH builds will be synchronized to first pass in the graph that requests their usage.
        void UseRayTracing();

        // Issue GPU -> CPU data transfer for subresources that are to be written in this render pass.
        // Can only be called for resource that is being written in requesting render pass.
        void Export(Foundation::Name resourceName);

        // To be called by the engine, not render passes
        void SetCurrentlySchedulingPassNode(RenderPassGraph::Node* node);
        void SetContent(const ContentMediator* content);

    private:
        NewTextureProperties FillMissingFields(std::optional<NewTextureProperties> properties) const;
        NewDepthStencilProperties FillMissingFields(std::optional<NewDepthStencilProperties> properties) const;
        uint32_t MaxMipCount(const Geometry::Dimensions& dimensions) const;

        void RegisterGraphDependency(
            RenderPassGraph::Node& passNode, 
            const MipSet& subresources, 
            Foundation::Name resourceName,
            Foundation::Name outputAliasName,
            uint32_t resourceMipCount, 
            bool isWriteDependency);

        void UpdateSubresourceInfos(
            PipelineResourceSchedulingInfo& resourceShcedulingInfo,
            const MipSet& mips,
            Foundation::Name passName,
            HAL::ResourceState state, 
            PipelineResourceSchedulingInfo::SubresourceInfo::AccessFlag accessFlag,
            std::optional<HAL::ColorFormat> concreteFormat);

        template <class Lambda>
        void FillCurrentPassInfo(const PipelineResourceStorageResource* resourceData, const MipList& mipList, const Lambda& lambda);

        RenderPassGraph::Node* mCurrentlySchedulingPassNode = nullptr;
        PipelineResourceStorage* mResourceStorage = nullptr;
        RenderPassUtilityProvider* mUtilityProvider = nullptr;
        RenderPassGraph* mRenderPassGraph = nullptr;
        const ContentMediator* mContent = nullptr;
        const PipelineSettings* mPipelineSettings = nullptr;

    public:
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mUtilityProvider->DefaultRenderSurfaceDescription; }
        inline auto GetFrameNumber() const { return mUtilityProvider->FrameNumber; }
        inline const ContentMediator* GetContent() const { return mContent; }
    };

}

ENABLE_BITMASK_OPERATORS(PathFinder::ResourceSchedulingFlags);

#include "ResourceScheduler.inl"