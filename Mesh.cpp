#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, std::vector<Vertex>* vertices) :
	vertexCount(vertices->size()), physicalDevice(newPhysicalDevice), logicalDevice(newLogicalDevice)
{
	createVertexBuffer(vertices);
}

Mesh::~Mesh()
{
	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
}

void Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	// CREATE VERTEX BUFFER
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(Vertex) * vertexCount;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkResult result = vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &vertexBuffer);
	checkResult(result, "Failed to create a Vertex Buffer");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDevice, vertexBuffer, &memRequirements);

	// ALLOCATE MEMORY TO BUFFER
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = memRequirements.size;
	memAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memRequirements.memoryTypeBits,			// VK_FIRST : CPU can interact with memory.
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);			// VK_SECOND : allows placement of data straight into buffer after mapping

	// ALLOCATE MEMORY TO VK DEVICE MEMORY
	result = vkAllocateMemory(logicalDevice, &memAllocInfo, nullptr, &vertexBufferMemory);
	checkResult(result, "Failed to allocate vertexbuffer");

	vkBindBufferMemory(logicalDevice, vertexBuffer, vertexBufferMemory, 0);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;																					// 1. Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, vertexBufferMemory, 0, bufferInfo.size, 0, &data);				// 2. Map vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferInfo.size);									// 3. copy memory from vertices vector to popint
	vkUnmapMemory(logicalDevice, vertexBufferMemory);											// 4. Unmap the vertex buffer memory
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
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
