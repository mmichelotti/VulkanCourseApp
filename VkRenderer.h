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

#include "stb_image.h"

#include "VulkanValidation.h"
#include "Utilities.h"
#include "Window.h"
#include "Mesh.h"



class VkRenderer
{
public:
	VkRenderer(const Window& window);
	void updateModel(size_t modelId, glm::mat4 newModel);
	void draw();
	~VkRenderer();

private:
	GLFWwindow* window;
	size_t currentFrame = 0;

	//Scene objects
	std::vector<Mesh*> meshes;

	UboViewProjection uboVP;

	// Vulkan Components
	VkInstance instance;
	VkDebugReportCallbackEXT callback;

	Device device;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;


	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;

	std::vector<SwapChainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	VkImage depthBufferImage;
	VkDeviceMemory depthBufferImageMemory;
	VkImageView depthBufferImageView;

	VkCommandPool graphicsCommandPool;

	VkPipeline graphicsPipeline;
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;





	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	std::vector<VkSemaphore> imageSemaphores;
	std::vector <VkSemaphore> renderSemaphores;
	std::vector <VkFence> drawFences;

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;			// 1 for each swap chain images
	std::vector<VkDescriptorSet> samplerDescriptorSets;		// 1 for each texture

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	VkSampler textureSampler;
	std::vector<VkImage> textureImages;
	std::vector<VkImageView> textureImageViews;
	std::vector<VkDeviceMemory> textureImageMemory;

#pragma region -- Create Functions --
	void createInstance();
	void createDebugCallback();
	void createLogicalDevice();
	void createSurface();
	void createSwapChain();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createPushConstantRange();
	void createGraphicsPipeline();
	void createDepthBufferImage();
	void createFrameBuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronization();
	void createMesh();
	void createTextureSampler();
	
	
	// - Create for descriptors
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();

	void updateUniformBuffers(uint32_t imgIndex);
#pragma endregion

	// - Record
	void recordCommands(uint32_t imageIndex);

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
	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);


	VkImage createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkShaderModule createShaderModule(const std::string& fileName);



	size_t createTextureImage(std::string fileName);
	size_t createTexture(std::string fileName);
	size_t createTextureDescriptor(VkImageView textureImage);

	//--loading
	stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* size);
};

