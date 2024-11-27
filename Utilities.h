#pragma once
#include <fstream>
#include <GLM\glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const size_t MAX_FRAME_DRAWS = 2;
struct Vertex
{
	glm::vec3 position;
	glm::vec3 color;
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
static void createCompleteBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, 
	VkMemoryPropertyFlags bufferProperties, VkBuffer* pBuffer, VkDeviceMemory* pBufferMemory)
{
	// CREATE VERTEX BUFFER
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = bufferSize;
	bufferInfo.usage = bufferUsage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, pBuffer);
	checkResult(result, "Failed to create a Vertex Buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, *pBuffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits, bufferProperties);

	// ALLOCATE MEMORY TO VK DEVICE MEMORY
	result = vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, pBufferMemory);
	checkResult(result, "Failed to allocate vertexbuffer");

	vkBindBufferMemory(logicalDevice, *pBuffer, *pBufferMemory, 0);
}
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