#pragma once

#include <IO/Input.hpp>
#include <Geometry/Dimensions.hpp>
#include <Memory/GPUResourceProducer.hpp>
#include <RenderPipeline/PipelineResourceStorage.hpp>

#include "UIGPUStorage.hpp"
#include "ViewController.hpp"

#include <windef.h>
#include <vector>

namespace PathFinder
{

    class UIManager 
    {
    public:
        UIManager(HWND windowHandle, Input* input, PipelineResourceStorage* resourceStorage, Memory::GPUResourceProducer* resourceProducer);

        void Draw();
        void SetViewportSize(const Geometry::Dimensions& size);

        bool IsInteracting() const;
        bool IsMouseOverUI() const;

        template <class ViewControllerT> 
        ViewControllerT* CreateViewController();

    private:
        void PollInputs();
        void UpdateCursor();

        std::vector<std::unique_ptr<ViewController>> mViewControllers;
        bool mIsInteracting = false;
        bool mIsMouseOverUI = false;
        HWND mWindowHandle;
        UIGPUStorage mGPUStorage;
        Input* mInput;
        PipelineResourceStorage* mResourceStorage;

    public:
        inline const UIGPUStorage& GPUStorage() const { return mGPUStorage; }
    };

    template <class ViewControllerT>
    ViewControllerT* UIManager::CreateViewController()
    {
        static_assert(std::is_base_of<ViewController, ViewControllerT>::value, "ViewControllerT must derive from ViewController");

        auto vcUniquePtr = std::make_unique<ViewControllerT>();

        vcUniquePtr->SetInput(mInput);
        vcUniquePtr->SetResourceStorage(mResourceStorage);
        vcUniquePtr->SetUIManager(this);

        auto vc = vcUniquePtr.get();
        mViewControllers.emplace_back(std::move(vcUniquePtr));
        
        return vc;
    }

}
