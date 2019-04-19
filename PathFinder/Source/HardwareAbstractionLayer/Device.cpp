#include "Device.hpp"
#include "Utils.h"

namespace HAL
{
	Device::Device(const DisplayAdapter& adapter)
	{
	#if defined(DEBUG) || defined(_DEBUG) 
	    // Enable the D3D12 debug layer.
	    {
	        Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
	        ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	        debugController->EnableDebugLayer();
	    }
	#endif

        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	    ThrowIfFailed(D3D12CreateDevice(adapter.COMPtr().Get(), featureLevel, IID_PPV_ARGS(&mDevice)));
	}
	
	Device::~Device()
	{
	
	}
	
    RTDescriptor Device::EmplaceDescriptorInHeap(const Resource& resource, const RTDescriptorHeap& heap)
    {
        return RTDescriptor(mDevice, resource.Resource(), heap.Heap());
    }

    //CommandAllocator Device::CreateCommandAllocator() const
	//{
	//    return CommandAllocator();
	//}
	//
	//CommandQueue Device::CreateCommandQueue() const
	//{
 //       return CommandQueue();
	//}
	//
	//Fence Device::CreateFence() const
	//{
 //       return Fence(mDevice);
	//}
}
