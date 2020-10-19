#pragma once

#include <cstdint>
#include <HardwareAbstractionLayer/RayTracingAccelerationStructure.hpp>
#include <Foundation/BitwiseEnum.hpp>

namespace PathFinder 
{
   
    using EntityID = uint32_t;

    inline static const auto NoEntityID = std::numeric_limits<EntityID>::max();

    enum class EntityMask : uint8_t
    {
        Unknown = 0, MeshInstance = 1 << 0, Light = 1 << 1
    };

    inline HAL::RayTracingTopAccelerationStructure::InstanceInfo RTASInstanceInfoForEntity(EntityID id, EntityMask mask)
    {
        return { id, std::underlying_type_t<EntityMask>(mask) };
    }

}

ENABLE_BITMASK_OPERATORS(PathFinder::EntityMask);