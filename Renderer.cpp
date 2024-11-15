#include "Renderer.h"

Renderer::Renderer(GLFWwindow* window) : window(window)
{
    try
    {
        instance = new Instance();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
    }
}

Renderer::~Renderer()
{
    delete instance;
}
