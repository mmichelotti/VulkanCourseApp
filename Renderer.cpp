#include "Renderer.h"

Renderer::Renderer()
{
}

int Renderer::Init(GLFWwindow* window)
{
    this->window = window;

    try
    {
        CreateInstance();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return 0;
}

void Renderer::CreateInstance()
{
    //Information about the app itelf, most of the data is useful for the development
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    //Create list to hold instance extensions
    std::vector<const char*> instanceExtensions = std::vector<const char*>();
    uint32_t glfwExtensionCount = 0;                                                             //GLFW may require multiple extensions
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);        //Extensions papsses as array of cstrings, so needs pointer (the array) to popinter (the cstring)
    for (size_t i = 0; i < glfwExtensionCount; i++)
    {
        instanceExtensions.push_back(glfwExtensions[i]);
    }

    //Creation information for a vulkan instance
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t> (instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

    //Create the instance
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance); //often gonna be nullptr and let vulkan handle memory

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vulkan Instance");
    }
}
