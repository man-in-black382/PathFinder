#pragma once

#include "../HardwareAbstractionLayer/BufferResource.hpp"
#include "../HardwareAbstractionLayer/TextureResource.hpp"
#include "../HardwareAbstractionLayer/Descriptor.hpp"
#include "../Foundation/Name.hpp"
#include "../Geometry/Dimensions.hpp"

namespace PathFinder
{

    template <class DescriptorT>
    class ResourceView
    {
    public:
        ResourceView(Foundation::Name resourceName, const DescriptorT& descriptor);

    private:
        Foundation::Name mResourceName;
        DescriptorT mDescriptor;

    public:
        Foundation::Name ResourceName() const { return mResourceName; }
        const DescriptorT& ResourceDescriptor() const { return mDescriptor; }
    };

    template <class DescriptorT>
    ResourceView<DescriptorT>::ResourceView(Foundation::Name resourceName, const DescriptorT& descriptor)
        : mResourceName{ resourceName }, mDescriptor{ descriptor } {}

}
