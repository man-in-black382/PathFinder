#pragma once

#include "RenderPassUtilityProvider.hpp"
#include "PipelineResourceStorage.hpp"

#include "../Foundation/Name.hpp"

namespace PathFinder
{

    template <class ContentMediator>
    class RenderPassContainer;

    template <class ContentMediator>
    class RenderSubPass;

    template <class ContentMediator>
    class SubPassScheduler
    {
    public:
        SubPassScheduler(const RenderPassContainer<ContentMediator>* passContainer, const PipelineResourceStorage* resourceStorage, const RenderPassUtilityProvider* utilityProvider);

        void AddRenderSubPass(const RenderSubPass<ContentMediator>* pass);

        const HAL::TextureProperties& GetTextureProperties(Foundation::Name textureName) const;
        const HAL::BufferProperties<uint8_t>& GetBufferProperties(Foundation::Name textureName) const;

    private:
        const PipelineResourceStorage* mResourceStorage;
        const RenderPassUtilityProvider* mUtilityProvider;
        const RenderPassContainer<ContentMediator>* mPassContainer;

    public:
        inline const RenderSurfaceDescription& GetDefaultRenderSurfaceDesc() const { return mUtilityProvider->DefaultRenderSurfaceDescription; }
        inline auto FrameNumber() const { return mUtilityProvider->FrameNumber; }
    };

}

#include "RenderPassContainer.hpp"
#include "SubPassScheduler.inl"