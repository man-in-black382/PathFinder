#include "RenderGraph.hpp"

namespace PathFinder
{

    

    void RenderGraph::WillRenderToRenderTarget(HAL::ResourceFormat::Color dataFormat, HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    void RenderGraph::WillRenderToRenderTarget(HAL::ResourceFormat::TypelessColor dataFormat, HAL::ResourceFormat::TextureKind kind, const Geometry::Dimensions& dimensions)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

    void RenderGraph::WillRenderToDepthStencil(HAL::ResourceFormat::DepthStencil dataFormat, const Geometry::Dimensions& dimensions)
    {
        throw std::logic_error("The method or operation is not implemented.");
    }

}
