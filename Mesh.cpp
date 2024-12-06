#include "Mesh.h"


Mesh::Mesh(Device device, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices, size_t texId) :
	device(device), texId(texId)
{
	vertex = MeshData(device, transferQueue, transferCmdPool, vertices, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	index = MeshData(device, transferQueue, transferCmdPool, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	modelMatrix = glm::mat4(1.0f);
}

Mesh::~Mesh()
{
	vkDestroyBuffer(device.logical, vertex.buffer, nullptr);
	vkFreeMemory(device.logical, vertex.bufferMemory, nullptr);
	vkDestroyBuffer(device.logical, index.buffer, nullptr);
	vkFreeMemory(device.logical, index.bufferMemory, nullptr);
}
