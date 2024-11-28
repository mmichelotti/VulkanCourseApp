#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"
class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices);
	~Mesh();


	size_t getVertexCount() { return vertexCount; }
	VkBuffer getVertexBuffer() { return vertexBuffer; }
	size_t getIndexCount() { return indexCount; }
	VkBuffer getIndexBuffer() { return indexBuffer; }

private:
	//vertex data
	size_t vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	//index data
	size_t indexCount;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices);
	void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices);
};

