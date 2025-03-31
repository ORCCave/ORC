#pragma	once

#include "OrcDefines.h"

namespace Orc
{
    class GraphicsDevice;
	class CommandList
	{
	public:
		enum class CommandListTypes
		{
			CLT_GRAPHICS,
			CLT_COPY,
			CLT_COMPUTE,
		};

		virtual void begin() = 0;
		virtual void end() = 0;

		virtual void* getRawCommandList() const = 0;

		ORC_DISABLE_COPY_AND_MOVE(CommandList)
	protected:
		CommandList() = default;
		virtual ~CommandList() = default;

        void* mInternalCommandPool;

        friend class GraphicsDevice;
	};
}