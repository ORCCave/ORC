#pragma once

#include "OrcDefines.h"

namespace Orc
{
    class GraphicsDevice;
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

        ORC_DISABLE_COPY_AND_MOVE(GraphicsCommandList)
    protected:
        GraphicsCommandList() = default;
        virtual ~GraphicsCommandList() = default;

        friend class GraphicsDevice;
    };
}