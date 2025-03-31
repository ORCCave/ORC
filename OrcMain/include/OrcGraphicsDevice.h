#pragma once

#include "OrcCommandList.h"
#include "OrcDefines.h"

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

        virtual CommandList* createCommandList(CommandList::CommandListTypes type) = 0;
        void destroyCommandList(CommandList* commandList) { delete commandList; }

        GraphicsDeviceTypes getGraphicsDeviceType() const { return mGraphicsDeviceType; }
        virtual void executeCommandList(CommandList::CommandListTypes type, uint32 numLists, CommandList* const* lists) = 0;

        ORC_DISABLE_COPY_AND_MOVE(GraphicsDevice)

    protected:
        GraphicsDevice(GraphicsDeviceTypes type) : mGraphicsDeviceType(type) {}
        virtual ~GraphicsDevice() = default;

        virtual void beginDraw() {}
        virtual void endDraw() = 0;

        GraphicsDeviceTypes mGraphicsDeviceType;

        friend class Root;
        friend class CommandList;
    };
}