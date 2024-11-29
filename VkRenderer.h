#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <GLM/glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <set>
#include <unordered_set>
#include <algorithm>
#include <array>

#include "VulkanValidation.h"
#include "Utilities.h"
#include "Window.h"
#include "Mesh.h"

class VkRenderer
{
public:
	VkRenderer(const Window& window);
	void updateModel(glm::mat4 newModel);
	void draw();
	~VkRenderer();

private:
	GLFWwindow* window;
	size_t currentFrame = 0;

	//Scene objects
	Mesh* firstMesh;
	std::vector<Mesh*> meshes;

	//Scene settings
	struct MVP
	{
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
	} mvp;

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

#pragma region Syncronization members
	std::vector<VkSemaphore> imageSemaphores;
	std::vector <VkSemaphore> renderSemaphores;
	std::vector <VkFence> drawFences;
#pragma endregion

#pragma region Descriptor members
	VkDescriptorSetLayout descriptorSetLayout;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkBuffer> uniformBuffer;
	std::vector<VkDeviceMemory> uniformBufferMemory;
#pragma endregion


#pragma region Functions - Create
	void createInstance();
	void createDebugCallback();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFrameBuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();
	void createMesh();
	void createMVP();

	
	// - Create for descriptors
	void createUniformBuffer();
	void createDescriptorPool();
	void createDescriptorSets();

	void updateUniformBuffer(uint32_t imgIndex);
#pragma endregion
	

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

