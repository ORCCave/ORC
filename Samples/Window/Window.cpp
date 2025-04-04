#include "Orc.h"

#include <exception>
#include <iostream>

int main()
{
    try
    {
        Orc::ApplicationContext ctx("OrcWindow", 800, 600);
        Orc::Root* root = ctx.getRoot();
        auto device = root->createGraphicsDevice(Orc::GraphicsDevice::GraphicsDeviceTypes::GDT_VULKAN);
        root->startRendering(device.get());
    }
    catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
    catch (...) { std::cerr << "Unknown exception caught." << std::endl; }

    return 0;
}