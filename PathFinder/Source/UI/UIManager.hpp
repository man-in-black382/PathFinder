#pragma once

#include <IO/Input.hpp>
#include <Geometry/Dimensions.hpp>
#include <Memory/GPUResourceProducer.hpp>
#include <robinhood/robin_hood.h>

#include "UIGPUStorage.hpp"

#include "ViewModel.hpp"

#include <windef.h>
#include <vector>
#include <typeindex>

namespace PathFinder
{

    class ViewController;

    class UIManager 
    {
    public:
        UIManager(Input* input, UIDependencies* dependencies, Memory::GPUResourceProducer* resourceProducer);
        ~UIManager();

        void Draw();
        void SetViewportSize(const Geometry::Dimensions& size);

        bool IsInteracting() const;
        bool IsMouseOverUI() const;

        template <class ViewControllerT> 
        std::shared_ptr<ViewControllerT> CreateViewController();

        template <class ViewModelT>
        ViewModelT* GetViewModel();

    private:
        using ViewModelTypeIndex = std::type_index;

        void HandleKeyUp(KeyboardKey key, const KeyboardKeyInfo& info, const Input* input);
        void HandleKeyDown(KeyboardKey key, const KeyboardKeyInfo& info, const Input* input);
        void UpdateCursor();

        std::vector<std::weak_ptr<ViewController>> mViewControllers;
        robin_hood::unordered_flat_map<ViewModelTypeIndex, std::unique_ptr<ViewModel>> mViewModels;

        bool mIsInteracting = false;
        bool mIsMouseOverUI = false;
        UIGPUStorage mGPUStorage;
        Input* mInput;
        UIDependencies* mUIDependencies;

    public:
        inline const UIGPUStorage& GPUStorage() const { return mGPUStorage; }
    };

}

#include "UIManager.inl"
