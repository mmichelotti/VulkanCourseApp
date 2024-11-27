#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"
class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, std::vector<Vertex>* vertices);
	~Mesh();


	size_t getVertexCount() { return vertexCount; }
	VkBuffer getVertexBuffer() { return vertexBuffer; }

private:
	size_t vertexCount;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	void createVertexBuffer(std::vector<Vertex>* vertices);
};

