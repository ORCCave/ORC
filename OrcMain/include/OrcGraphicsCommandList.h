#pragma once

#include "OrcDefines.h"

namespace Orc
{
    class GraphicsCommandList
    {
    public:
        enum class GraphicsCommandListType
        {
            GCLT_GRAPHICS,
            GCLT_COPY,
            GCLT_COMPUTE,
        };

        virtual void begin() = 0;
        virtual void end() = 0;

        virtual void* getRawCommandList() const = 0;

        GraphicsCommandListType getCommandListType() const { return mType; }

        ORC_DISABLE_COPY_AND_MOVE(GraphicsCommandList)
    protected:
        GraphicsCommandList(GraphicsCommandListType type) : mType(type) {}
        virtual ~GraphicsCommandList() = default;

        bool mIsUsable = true;

        GraphicsCommandListType mType;
    };
}