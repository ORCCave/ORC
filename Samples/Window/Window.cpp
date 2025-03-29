#include "Orc.h"

int main()
{
    Orc::ApplicationContext ctx("OrcWindow", 800, 600);
    Orc::Root* root = ctx.getRoot();
    Orc::GraphicsDevice* device = root->createGraphicsDevice(Orc::GraphicsDevice::GraphicsDeviceTypes::GDT_D3D12);
    root->startRendering(device);
    root->destroyGraphicsDevice(device);
    return 0;
}