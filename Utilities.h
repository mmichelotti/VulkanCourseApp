#pragma once
#include <fstream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <GLM\glm.hpp>
#include <GLM/gtc/matrix_transform.hpp>

const size_t MAX_FRAME_DRAWS = 2;
const size_t MAX_OBJECTS = 2;

struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
};
struct UboViewProjection
{
	glm::mat4 projection;
	glm::mat4 view;

	UboViewProjection() : projection(glm::mat4(1.0f)), view(glm::mat4(1.0f)) {}
	UboViewProjection(float width, float height)
	{
		projection = glm::perspective(glm::radians(45.0f), width / height, 0.01f, 100.0f);
		view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		projection[1][1] *= -1; // Vulkan inverts the Y 
	}
};

struct Device
{
	VkPhysicalDevice physical;
	VkDevice logical;
};
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static void checkResult(const VkResult& result, const char* errorMessage)
{
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error(errorMessage);
	}
}
// Indices (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndices {
	int graphicsFamily = -1;			// Location of Graphics Queue Family
	int presentationFamily = -1;		// Location of Presentation Queue Family

	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;		// Surface properties, e.g. image size/extent
	std::vector<VkSurfaceFormatKHR> formats;			// Surface image formats, e.g. RGBA and size of each colour
	std::vector<VkPresentModeKHR> presentationModes;	// How images should be presented to screen
};
struct SwapChainImage
{
	VkImage image;
	VkImage imageView;
};
static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	//Get the properties of physical device memory
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((allowedTypes & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
}
struct VkCompleteBufferInfo
{
	VkDeviceSize bufferSize;
	VkBufferUsageFlags bufferUsage;
	VkMemoryPropertyFlags bufferProperties;
	VkBuffer* pBuffer;
	VkDeviceMemory* pBufferMemory;
};

static void createCompleteBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkCompleteBufferInfo* completeBufferInfo)
{
	// CREATE VERTEX BUFFER
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = completeBufferInfo->bufferSize;
	bufferInfo.usage = completeBufferInfo->bufferUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, completeBufferInfo->pBuffer);
	checkResult(result, "Failed to create a Vertex Buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, *completeBufferInfo->pBuffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, completeBufferInfo->bufferProperties);

	// ALLOCATE MEMORY TO VK DEVICE MEMORY
	result = vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, completeBufferInfo->pBufferMemory);
	checkResult(result, "Failed to allocate vertexbuffer");

	vkBindBufferMemory(logicalDevice, *completeBufferInfo->pBuffer, *completeBufferInfo->pBufferMemory, 0);
}
static std::vector<char> readFile(const std::string& fileName)
{
	//std:binary tells to read file as binary
	//std::ate tells to start reading from END (At The End)
	std::ifstream file(fileName, std::ios::binary | std::iostream::ate);

	//Check if file stream success
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);
	file.seekg(0);								// Move read position back to 0 (at the start)
	file.read(fileBuffer.data(), fileSize);
	file.close();
	return fileBuffer;
}
static VkCommandBuffer beginCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool)
{
	// Command buffer to hold transfer commands
	VkCommandBuffer commandBuffer;

	// Command Buffer details
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

	// Information to begin the command buffer record
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// We're only using the command buffer once, so set up for one time submit

	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}
static void endAndSubmitCommandBuffer(VkDevice logicalDevice, VkCommandPool cmdPool, VkQueue queue, VkCommandBuffer cmdBuffer)
{
	// End commands
	vkEndCommandBuffer(cmdBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &cmdBuffer;

	// Submit transfer command to transfer queue and wait until it finishes
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Free temporary command buffer back to pool
	vkFreeCommandBuffers(logicalDevice, cmdPool, 1, &cmdBuffer);
}
static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCmdPool,
	VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	// Create Buffer
	VkCommandBuffer transferCmdBuffer = beginCommandBuffer(device, transferCmdPool);

	// Region of data to copy from and to
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCmdBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	endAndSubmitCommandBuffer(device, transferCmdPool, transferQueue, transferCmdBuffer);

}
static void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCmdPool, VkBuffer srcBuffer, VkImage dstImage, uint32_t width, uint32_t height)
{
	VkCommandBuffer transferCmdBuffer = beginCommandBuffer(device, transferCmdPool);

	VkBufferImageCopy imageCopyRegion{};
	imageCopyRegion.bufferOffset = 0;
	imageCopyRegion.bufferRowLength = 0;
	imageCopyRegion.bufferImageHeight = 0;
	imageCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageCopyRegion.imageSubresource.mipLevel = 0;
	imageCopyRegion.imageSubresource.baseArrayLayer = 0;
	imageCopyRegion.imageSubresource.layerCount = 1;
	imageCopyRegion.imageOffset = { 0,0,0 };
	imageCopyRegion.imageExtent = { width,height,1 };

	vkCmdCopyBufferToImage(transferCmdBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyRegion);

	endAndSubmitCommandBuffer(device, transferCmdPool, transferQueue, transferCmdBuffer);
}
static void createBuffer(Device device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
	VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	// CREATE VERTEX BUFFER
	// Information to create a buffer (doesn't include assigning memory)
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;								// Size of buffer (size of 1 vertex * number of vertices)
	bufferInfo.usage = bufferUsage;								// Multiple types of buffer possible
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Similar to Swap Chain images, can share vertex buffers

	VkResult result = vkCreateBuffer(device.logical, &bufferInfo, nullptr, buffer);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vertex Buffer!");
	}

	// GET BUFFER MEMORY REQUIREMENTS
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device.logical, *buffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(device.physical, memRequirements.memoryTypeBits,		// Index of memory type on Physical Device that has required bit flags
		bufferProperties);																						// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT	: CPU can interact with memory
	// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(device.logical, &memoryAllocInfo, nullptr, bufferMemory);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(device.logical, *buffer, *bufferMemory, 0);
}

static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool cmdPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer cmdBuffer = beginCommandBuffer(device, cmdPool);

	// this stage has to finish before that other stage can begin
	VkImageMemoryBarrier imgMemBarrier{};
	imgMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imgMemBarrier.oldLayout = oldLayout;
	imgMemBarrier.newLayout = newLayout;
	imgMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;				// dont bother queue transition of queue family
	imgMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imgMemBarrier.image = image;
	imgMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imgMemBarrier.subresourceRange.baseMipLevel = 0;
	imgMemBarrier.subresourceRange.levelCount = 1;
	imgMemBarrier.subresourceRange.baseArrayLayer = 0;
	imgMemBarrier.subresourceRange.layerCount = 1;


	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	// if transition from new img to img ready to receive data...
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		// the transition between old and new layout NEEDS to happpen before this very stage VK_ACCESS_TRANSFER_WRITE_BIT
		imgMemBarrier.srcAccessMask = 0;									// after
		imgMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;			// before
		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// if transitioning from transfer destination to shader readable...
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		imgMemBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imgMemBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier
	(
		cmdBuffer,
		srcStage, dstStage,																// pipeline stages (match to src and dst accessmasks)
		0,																	// dependency flags
		0, nullptr,															// mem barrier count and data
		0, nullptr,															// buff mem barrier count and data
		1, &imgMemBarrier													// img mem barrier count and data
	);


	endAndSubmitCommandBuffer(device, cmdPool, queue, cmdBuffer);
}