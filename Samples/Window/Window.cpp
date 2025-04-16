#include "Orc.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        Orc::ApplicationContext ctx("OrcWindow", 800, 600);
        auto* root = ctx.getRoot();
        auto device = root->getGraphicsDevice(Orc::GraphicsDevice::GraphicsDeviceType::GDT_VULKAN);
        root->startRendering(device);
    }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
    catch (...) { std::cerr << "Unknown exception caught." << std::endl; }

    return 0;
}