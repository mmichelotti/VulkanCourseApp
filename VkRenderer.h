#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <unordered_set>

#include "VulkanValidation.h"
#include "Utilities.h"
#include "Window.h"

class VkRenderer
{
public:
	VkRenderer(const Window& window);
	~VkRenderer();

private:
	GLFWwindow* window;

	// Vulkan Components
	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	struct 
	{
		VkPhysicalDevice physicalDevice;
		VkDevice logicalDevice;
	} mainDevice;
	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;


	// Vulkan Functions
	// - Create Functions
	void createInstance();
	void createDebugCallback();
	void createLogicalDevice();
	void createSurface();

	// - Get Functions
	void getPhysicalDevice();

	// - Support Functions
	// -- Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	bool checkDeviceSuitable(VkPhysicalDevice device);

	// -- Getter Functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	
	std::unordered_set<std::string> GetInstanceExtensions();
};

