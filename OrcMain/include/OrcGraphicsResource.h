#pragma once

#include "OrcDefines.h"

namespace Orc
{
    class GraphicsResource
    {
    public:
        enum class GraphicsResourceType
        {
            GRT_BUFFER,
            GRT_TEXTURE,
        };

        enum class GraphicsResourceState
        {
            GRS_COMMON = 0,
            GRS_PRESENT,
            GRS_RENDER_TARGET,
        };

        virtual GraphicsResourceType getType() const = 0;

        ORC_DISABLE_COPY_AND_MOVE(GraphicsResource)
    protected:
        GraphicsResource() = default;
        virtual ~GraphicsResource() = default;

    };

    class GraphicsBuffer : public GraphicsResource
    {
    public:
        GraphicsResourceType getType() const override { return GraphicsResourceType::GRT_BUFFER; }

        ORC_DISABLE_COPY_AND_MOVE(GraphicsBuffer)
    protected:
        GraphicsBuffer() = default;
        ~GraphicsBuffer() = default;
    };

    class GraphicsTexture : public GraphicsResource
    {
    public:
        enum class GraphicsTextureFormat
        {
            GTF_UNDEFINED = 0,
            GTF_R8G8B8A8_UNORM,
        };

        GraphicsResourceType getType() const override { return GraphicsResourceType::GRT_TEXTURE; }

        ORC_DISABLE_COPY_AND_MOVE(GraphicsTexture)
    protected:
        GraphicsTexture() = default;
        ~GraphicsTexture() = default;
    };
}