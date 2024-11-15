#include "Renderer.h"


Renderer::Renderer(GLFWwindow* window)
{
    this->window = window;
    try
    {
        CreateInstance();
        GetPhysicalDevice();
        CreateLogicalDevice();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
    }
}

Renderer::~Renderer()
{
    vkDestroyDevice(device.logical, nullptr);
    vkDestroyInstance(instance, nullptr);
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
    
    if (!CheckInstanceExtensionSupport(&instanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support required extensions!");
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

void Renderer::CreateLogicalDevice()
{
    QueueFamilyIndices indices = GetQueueFamilies(device.physical);
    //Queue the logical device needs to create and info to do so
    //only 1 for now, to do later 
    VkDeviceQueueCreateInfo queueCrateInfo = {};
    queueCrateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    queueCrateInfo.queueFamilyIndex = indices.graphicsFamily;
    queueCrateInfo.queueCount = 1;

    float priority = 1.0f;
    queueCrateInfo.pQueuePriorities = &priority;

    //Physical device features the logical device will be using
    VkPhysicalDeviceFeatures deviceFeatures = {};

    //Logical device create info
    VkDeviceCreateInfo logicalDeviceCreateInfo = {};
    logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceCreateInfo.queueCreateInfoCount = 1;
    logicalDeviceCreateInfo.pQueueCreateInfos = &queueCrateInfo;
    logicalDeviceCreateInfo.enabledExtensionCount = 0;
    logicalDeviceCreateInfo.ppEnabledExtensionNames = nullptr;
    logicalDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    //Creation of the logical device given the physical one
    VkResult result = vkCreateDevice(device.physical, &logicalDeviceCreateInfo, nullptr, &device.logical);
    if (result != VK_SUCCESS) 
    {
        throw std::runtime_error("Failed to create Logical Device!");
    }

    //Queues are created at the same time as the logical device
    //So we want to handle to queues
    //From given logical device of given queue family of given queue index (0 since only 1 queue), place reference in vkQueue
    vkGetDeviceQueue(device.logical, indices.graphicsFamily, 0, &graphicsQueue);
}

bool Renderer::CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
    std::unordered_set<std::string> availableExtensions = GetInstanceExtensions();
    for (const char* checkExtension : *checkExtensions)
    {
        if (availableExtensions.find(checkExtension) == availableExtensions.end()) return false;
    }
    return true;
}

bool Renderer::CheckDeviceSuitable(VkPhysicalDevice device)
{
    /*
    //Information about the device itself
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    //Information about what the device can do
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);
    */

    return GetQueueFamilies(device).isValid();
}

std::unordered_set<std::string> Renderer::GetInstanceExtensions()
{
    //Count the number of extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    //Create of vkextensions based on the amount we have
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    //Save it on a set of strings for faster querying
    std::unordered_set<std::string> set;
    for (const VkExtensionProperties& extension : extensions)
    {
        set.insert(extension.extensionName);
    }
    return set;
}

void Renderer::GetPhysicalDevice()
{
    //Enumerate Physical devices the vkInstance can access
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

    if (physicalDeviceCount == 0)
    {
        throw std::runtime_error("Can't find GPUs that support Vulkan Instance");
    }

    //Create a device list based on the amount we have
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());

    for (const auto& physicalDevice : physicalDevices)
    {
        if (CheckDeviceSuitable(physicalDevice))
        {
            device.physical = physicalDevice;
            break;
        }
    }
}

QueueFamilyIndices Renderer::GetQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());


    int i = 0;
    for (const auto& queueFamily : queueFamilyList)
    {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        if (indices.isValid()) break;
        i++;
    }
    return indices;
}
