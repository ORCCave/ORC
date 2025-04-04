#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsCommandList.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

namespace Orc
{
    class Root;
    class GraphicsDevice
    {
    public:
        enum class GraphicsDeviceTypes
        {
            GDT_D3D12,
            GDT_VULKAN,
        };

        virtual void* getRawGraphicsDevice() const = 0;
        virtual void clearSwapChainColor(GraphicsCommandList* list, float r, float g, float b, float a) = 0;

        virtual std::shared_ptr<GraphicsCommandList> createCommandList(GraphicsCommandList::GraphicsCommandListTypes type) = 0;

        virtual void executeCommandList(GraphicsCommandList::GraphicsCommandListTypes type, uint32 numLists, GraphicsCommandList* const* lists) = 0;

        ORC_DISABLE_COPY_AND_MOVE(GraphicsDevice)

    protected:
        GraphicsDevice(GraphicsDeviceTypes type) : mGraphicsDeviceType(type) {}
        virtual ~GraphicsDevice() = default;

        virtual void beginDraw() {}
        virtual void endDraw() = 0;

        GraphicsDeviceTypes mGraphicsDeviceType;

        friend class Root;
        friend class GraphicsCommandList;
    };
}