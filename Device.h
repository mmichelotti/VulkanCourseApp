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
	VkQueue graphicsQueue;

	VkPhysicalDevice physical;
	VkDevice logical;

	void PickPhysicalDevice(const VkInstance& instance);
	void InitializeLogicalDevice();

	bool CheckDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);
};

