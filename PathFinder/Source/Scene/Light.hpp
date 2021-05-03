#pragma once

#include <Foundation/Color.hpp>
#include <glm/mat4x4.hpp>

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
        void SetPosition(const glm::vec3& position);

        void SetIndexInGPUTable(uint32_t index);
        void SetVertexStorageLocation(const VertexStorageLocation& location);

        void UpdatePreviousFrameValues();
        virtual void ConstructModelMatrix() = 0;

    protected:
        void SetArea(float area);

        glm::vec3 mPosition;
        glm::vec3 mPreviousPosition;
        glm::mat4 mModelMatrix;
        uint32_t mIndexInGPUTable = 0;

    private:
        Lumen mLuminousPower = 0.0;
        Nit mLuminance = 0.0;
        Nit mPreviousLuminance = 0.0;
        Foundation::Color mColor = Foundation::Color::White();
        float mArea = 0.0;
        float mPreviousArea = 0.0;
        VertexStorageLocation mVertexStorageLocation;

    public:
        inline const glm::vec3& GetPosition() const { return mPosition; }
        inline const glm::vec3& GetPreviousPosition() const { return mPreviousPosition; }
        inline float GetArea() const { return mArea; }
        inline float GetPreviousArea() const { return mPreviousArea; }
        inline Lumen GetLuminousPower() const { return mLuminousPower; }
        inline Nit GetLuminance() const { return mLuminance; }
        inline Nit GetPreviousLuminance() const { return mPreviousLuminance; }
        inline const Foundation::Color& GetColor() const { return mColor; }
        inline const glm::mat4& GetModelMatrix() const { return mModelMatrix; }
        inline auto GetIndexInGPUTable() const { return mIndexInGPUTable; }
        inline const VertexStorageLocation& GetLocationInVertexStorage() const { return mVertexStorageLocation; }
    };

}
