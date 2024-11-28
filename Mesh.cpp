#include "Mesh.h"


Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices) :
	physicalDevice(newPhysicalDevice), logicalDevice(newLogicalDevice)
{
	vertex = MeshData(newPhysicalDevice, newLogicalDevice, transferQueue, transferCmdPool, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	index = MeshData(newPhysicalDevice, newLogicalDevice, transferQueue, transferCmdPool, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

Mesh::~Mesh()
{
	vkDestroyBuffer(logicalDevice, vertex.buffer, nullptr);
	vkFreeMemory(logicalDevice, vertex.bufferMemory, nullptr);
	vkDestroyBuffer(logicalDevice, index.buffer, nullptr);
	vkFreeMemory(logicalDevice, index.bufferMemory, nullptr);
}
