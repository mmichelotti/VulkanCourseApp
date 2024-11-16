#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window) : window(window)
{
    try
    {
        instance = new Instance();
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
