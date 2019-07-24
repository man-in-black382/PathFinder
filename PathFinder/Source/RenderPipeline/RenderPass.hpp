#pragma once

#include "../Foundation/Name.hpp"

#include "IResourceScheduler.hpp"
#include "IResourceProvider.hpp"
#include "GraphicsDevice.hpp"

#include "RenderPasses/ResourceNameList.hpp"

namespace PathFinder
{

    class RenderPass
    {
    public:
        RenderPass(Foundation::Name name, const std::string& vsFileName, const std::string& psFileName);
        RenderPass(Foundation::Name name, const std::string& vsFileName, const std::string& gsFileName, const std::string& psFileName);
        RenderPass(Foundation::Name name, const std::string& csFileName);

        virtual void SetupPipelineStates() = 0;
        virtual void ScheduleResources(IResourceScheduler* scheduler) = 0;
        virtual void Render(IResourceProvider* resourceProvider, GraphicsDevice* device) = 0;

    private:
        Foundation::Name mName;
        std::string mVertexShaderFileName;
        std::string mPixelShaderFileName;
        std::string mGeometryShaderFileName;
        std::string mComputeShaderFileName;

    public:
        inline Foundation::Name Name() const { return mName; }
        inline const std::string& VertexShaderFileName() const { return mVertexShaderFileName; }
        inline const std::string& PixelShaderFileName() const { return mPixelShaderFileName; }
        inline const std::string& GeometryShaderFileName() const { return mGeometryShaderFileName; }
        inline const std::string& ComputeShaderFileName() const { return mComputeShaderFileName; }
    };

}
