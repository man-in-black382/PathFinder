#include "MeshGPUStorage.hpp"

#include <algorithm>
#include <iterator>

namespace PathFinder
{

    MeshGPUStorage::MeshGPUStorage(const HAL::Device* device, Memory::GPUResourceProducer* resourceProducer)
        : mDevice{ device }, mResourceProducer{ resourceProducer }, mTopAccelerationStructure{ device, resourceProducer } {}        

}
