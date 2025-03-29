#include "Orc.h"

int main()
{
    Orc::ApplicationContext ctx("OrcWindow", 800, 600);
    ctx.initApp();
    Orc::Root* root = ctx.getRoot();
    Orc::GraphicsDevice* device = root->createGraphicsDevice(Orc::GraphicsDevice::GraphicsDeviceTypes::GDT_D3D12);
    root->startRendering(device);
    root->destroyGraphicsDevice(device);
    ctx.closeApp();
    return 0;
}