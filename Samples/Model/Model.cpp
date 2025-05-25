#include "OrcApplicationContext.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        Orc::ApplicationContext ctx(L"OrcWindow", 800, 600);
        auto root = ctx.getRoot();
        auto scene = root->createSceneManager("Scene");
        auto model = scene->createEntity("Model", "Models/BoxTextured.gltf");
        root->startRendering();
    }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
    catch (...) { std::cerr << "Unknown exception caught." << std::endl; }

    return 0;
}