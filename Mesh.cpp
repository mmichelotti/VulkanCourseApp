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
	
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();
	createCompleteBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, &vertexBuffer, &vertexBufferMemory);
	// MAP MEMORY TO VERTEX BUFFER
	void* data;																					// 1. Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, vertexBufferMemory, 0, bufferSize, 0, &data);				// 2. Map vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);									// 3. copy memory from vertices vector to popint
	vkUnmapMemory(logicalDevice, vertexBufferMemory);											// 4. Unmap the vertex buffer memory
}
