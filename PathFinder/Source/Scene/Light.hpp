#pragma once

#include <Foundation/Color.hpp>
#include <glm/mat4x4.hpp>

#include "EntityID.hpp"
#include "VertexStorageLocation.hpp"

namespace PathFinder 
{
    using Lumen = float;
    using Nit = float;
    using Kelvin = float;
    using Candela = float;

    class Light
    {
    public:
        virtual ~Light() = 0;

        void SetColor(const Foundation::Color& color);
        void SetColorTemperature(Kelvin temperature);

        /// Sets Luminous Power a.k.a Luminous Flux.
        /// Measure of the perceived power of light.
        void SetLuminousPower(Lumen luminousPower);

        void SetIndexInGPUTable(uint32_t index);
        void SetEntityID(EntityID id);
        void SetVertexStorageLocation(const VertexStorageLocation& location);

    protected:
        void SetArea(float area);

        glm::mat4 mModelMatrix;
        EntityID mEntityID = 0;
        uint32_t mIndexInGPUTable = 0;

    private:
        Lumen mLuminousPower = 0.0;
        Nit mLuminance = 0.0;
        Foundation::Color mColor = Foundation::Color::White();
        float mArea = 0.0;
        VertexStorageLocation mVertexStorageLocation;

    public:
        inline Lumen LuminousPower() const { return mLuminousPower; }
        inline Nit Luminance() const { return mLuminance; }
        inline const Foundation::Color& Color() const { return mColor; }
        inline const glm::mat4& ModelMatrix() const { return mModelMatrix; }
        inline const EntityID& ID() const { return mEntityID; }
        inline auto IndexInGPUTable() const { return mIndexInGPUTable; }
        inline const VertexStorageLocation& LocationInVertexStorage() const { return mVertexStorageLocation; }
    };

}
