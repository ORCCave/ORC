#include "Orc.h"

#include <iostream>

int main()
{
    try
    {
        Orc::ApplicationContext ctx("OrcWindow", 800, 600);
        Orc::Root* root = ctx.getRoot();
        auto device = root->createGraphicsDevice(Orc::GraphicsDevice::GraphicsDeviceTypes::GDT_D3D12);
        root->startRendering(device.get());
    }
    catch (const Orc::OrcException& ex)
    {
        std::cout << ex.what() << std::endl;
    }

    return 0;
}