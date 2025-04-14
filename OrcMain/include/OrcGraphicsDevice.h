#pragma once

#include "OrcDefines.h"
#include "OrcGraphicsCommandList.h"
#include "OrcGraphicsResource.h"
#include "OrcStdHeaders.h"
#include "OrcTypes.h"

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
        virtual void clearSwapChainColor(float r, float g, float b, float a) = 0;

        virtual std::shared_ptr<GraphicsCommandList> createCommandList(GraphicsCommandList::GraphicsCommandListType type) = 0;
        virtual GraphicsCommandList* getInternalCommandList(GraphicsCommandList::GraphicsCommandListType type) const = 0;

        virtual void executeCommandList(GraphicsCommandList::GraphicsCommandListType type, uint32 numLists, GraphicsCommandList* const* lists) = 0;

        ORC_DISABLE_COPY_AND_MOVE(GraphicsDevice)

    protected:
        GraphicsDevice() {}
        virtual ~GraphicsDevice() = default;

        virtual void beginDraw() = 0;
        virtual void endDraw() = 0;

        friend class Root;
        friend class GraphicsCommandList;
    };
}