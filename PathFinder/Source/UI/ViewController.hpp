#pragma once

#include <IO/Input.hpp>
#include <RenderPipeline/PipelineResourceStorage.hpp>

namespace PathFinder
{
   
    class UIManager;

    class ViewController
    {
    public:
        virtual void Draw() = 0;
        virtual bool IsInteracting() const { return false; }

        void SetUIManager(const UIManager* uiManager) { mUIManager = uiManager; }
        void SetInput(const Input* input) { mInput = input; }
        void SetResourceStorage(const PipelineResourceStorage* storage) { mResourceStorage = storage; }

    protected:
        const UIManager* mUIManager = nullptr;
        const Input* mInput = nullptr;
        const PipelineResourceStorage* mResourceStorage = nullptr;
    };

}
