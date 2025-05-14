#pragma once

#include "OrcDefines.h"
#include "OrcManager.h"
#include "OrcTypes.h"

#include <memory>
#include <vector>

namespace Orc
{
    class Root
    {
    public:
        void startRendering();

        SceneManager* createSceneManager(const String& sceneManagerName);
        void destrotSceneManager(SceneManager* sceneManager)
        {
            for (auto it = mSceneManagers.begin(); it != mSceneManagers.end(); ++it)
            {
                if (sceneManager == it->get())
                {
                    mSceneManagers.erase(it);
                    break;
                }
            }
        }

        ORC_DISABLE_COPY_AND_MOVE(Root)
    protected:
        Root(void* handle, uint32 w, uint32 h);

        ~Root() = default;

        uint32 mWidthForSwapChain;
        uint32 mHeightForSwapChain;

        std::shared_ptr<void> mGraphicsDevice;
        std::vector<std::shared_ptr<SceneManager>> mSceneManagers;
    };
}