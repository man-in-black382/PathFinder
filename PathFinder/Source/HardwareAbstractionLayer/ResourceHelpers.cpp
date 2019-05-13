#include "ResourceHelpers.hpp"
#include "Utils.h"

#include "../Foundation/Visitor.hpp"

namespace HAL
{

    //D3D12_RESOURCE_STATES D3DResourceStatesFromList(std::initializer_list<ReadTextureResourceState> states)
    //{
    //    D3D12_RESOURCE_STATES d3dStates;
    //    for (auto state : states) {
    //        std::visit(Foundation::MakeVisitor(
    //            [&d3dStates](ResourceState<ResourceUsage::Read> s) { d3dStates |= s.D3DState(); },
    //            [&d3dStates](TextureResourceState<ResourceUsage::Read> s) { d3dStates |= s.D3DState(); },
    //            state)
    //        );
    //    }
    //    return d3dStates;
    //}

    //D3D12_RESOURCE_STATES D3DResourceStatesFromList(std::initializer_list<ReadDepthStencilTextureResourceState> states)
    //{
    //    D3D12_RESOURCE_STATES d3dStates;
    //    for (auto state : states) {
    //        std::visit(Foundation::MakeVisitor(
    //            [&d3dStates](ResourceState<ResourceUsage::Read> s) { d3dStates |= s.D3DState(); },
    //            [&d3dStates](DepthStencilTextureResourceState<ResourceUsage::Read> s) { d3dStates |= s.D3DState(); },
    //            state)
    //        );
    //    }
    //    return d3dStates;
    //}

    //D3D12_RESOURCE_STATES D3DResourceStatesFromList(std::initializer_list<ReadBufferResourceState> states) {

    //}

    //D3D12_RESOURCE_STATES D3DResourceStates(WriteTextureResourceState states) {

    //}

    //D3D12_RESOURCE_STATES D3DResourceStates(WriteDepthStencilTextureResourceState states) {

    //}

    //D3D12_RESOURCE_STATES D3DResourceStates(WriteBufferResourceState states) {

    //}

    //D3D12_RESOURCE_STATES D3DResourceStates(ReadWriteTextureResourceState states) {

    //}

    //D3D12_RESOURCE_STATES D3DResourceStates(ReadWriteDepthStencilTextureResourceState states) {

    //}

    //D3D12_RESOURCE_STATES D3DResourceStates(ReadWriteBufferResourceState states) {

    //}

}
