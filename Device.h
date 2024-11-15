#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <vector>
#include "Utilities.h"
class Device
{
public:
	Device(const VkInstance& instance);
	~Device();
private:
	VkInstance instance;
	VkQueue graphicsQueue;

	VkPhysicalDevice physical;
	VkDevice logical;

	void InitializePhysicalDevice();
	void InitializeLogicalDevice();
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
};

