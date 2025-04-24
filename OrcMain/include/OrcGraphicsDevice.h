#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsCommandList.h"
#include "OrcStdHeaders.h"

namespace Orc
{
    class Root;
    class GraphicsDevice
    {
    public:
        enum class GraphicsDeviceType
        {
            GDT_D3D12,
            GDT_VULKAN,
        };

        virtual void* getRawGraphicsDevice() const = 0;

        virtual void executeCommandList(GraphicsCommandList* list) = 0;

        GraphicsDeviceType getGraphicsDeviceType() const { return mGraphicsDeviceType; }

        ORC_DISABLE_COPY_AND_MOVE(GraphicsDevice)

    protected:
        GraphicsDevice(GraphicsDeviceType type) : mGraphicsDeviceType(type) {}
        virtual ~GraphicsDevice() = default;

        virtual void beginDraw() = 0;
        virtual void endDraw() = 0;

        GraphicsDeviceType mGraphicsDeviceType;

        virtual std::shared_ptr<GraphicsCommandList> createCommandList(GraphicsCommandList::GraphicsCommandListType type) = 0;

        friend class Root;
        friend class GraphicsCommandList;
    };
}