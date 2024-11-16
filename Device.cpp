#include "Device.h"

Device::Device(const VkInstance& instance)
{
    PickPhysicalDevice(instance);
    InitializeLogicalDevice();
}

Device::~Device()
{
    vkDestroyDevice(logical, nullptr);
}

void Device::PickPhysicalDevice(const VkInstance& instance)
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
            physical = physicalDevice;
            break;
        }
    }
}

void Device::InitializeLogicalDevice()
{
    QueueFamilyIndices indices = GetQueueFamilies(physical);
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
    VkResult result = vkCreateDevice(physical, &logicalDeviceCreateInfo, nullptr, &logical);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Logical Device!");
    }

    //Queues are created at the same time as the logical device
    //So we want to handle to queues
    //From given logical device of given queue family of given queue index (0 since only 1 queue), place reference in vkQueue
    vkGetDeviceQueue(logical, indices.graphicsFamily, 0, &graphicsQueue);
}

bool Device::CheckDeviceSuitable(VkPhysicalDevice device)
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

QueueFamilyIndices Device::GetQueueFamilies(VkPhysicalDevice device)
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
