#include "Renderer.h"

Renderer::Renderer(Window& window)
{
    try
    {
        instance = new Instance();
        surface = instance->CreateSurface(window.GetWindow());
        device = new Device(instance->GetInstance());
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
    }
}

Renderer::~Renderer()
{
    delete device;
    delete instance;
}
