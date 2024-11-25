#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <array>

#include "VulkanValidation.h"
#include "Utilities.h"
#include "Window.h"

class VkRenderer
{
public:
	VkRenderer(const Window& window);
	void draw();
	~VkRenderer();

private:
	GLFWwindow* window;
	size_t currentFrame = 0;

	// Vulkan Components
	VkInstance instance;
	VkDebugReportCallbackEXT callback;
	struct 
	{
		VkPhysicalDevice physical;
		VkDevice logical;
	} device;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;


	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapChainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	VkCommandPool graphicsCommandPool;

	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkSemaphore> imageSemaphores;
	std::vector <VkSemaphore> renderSemaphores;
	std::vector <VkFence> drawFences;


	// Vulkan Functions
	// - Create Functions
	void createInstance();
	void createDebugCallback();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFrameBuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();

	// - Record
	void recordCommands();

	// - Get Functions
	void getPhysicalDevice();

	// - Support Functions
	// -- Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	bool checkValidationLayerSupport();
	bool checkDeviceSuitable(VkPhysicalDevice device);
	void checkResult(const VkResult& result, const char* errorMessage);


	// -- Getter Functions
	QueueFamilyIndices getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);

	std::unordered_set<std::string> GetInstanceExtensions();

	// - Choose funcitons
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::string& fileName);
};

