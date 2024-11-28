#include "Mesh.h"

Mesh::Mesh()
{
}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newLogicalDevice, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices) :
	vertexCount(vertices->size()), indexCount(indices->size()), physicalDevice(newPhysicalDevice), logicalDevice(newLogicalDevice)
{
	createVertexBuffer(transferQueue, transferCmdPool, vertices);
	createIndexBuffer(transferQueue, transferCmdPool, indices);
}

Mesh::~Mesh()
{
	vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMemory, nullptr);
	vkDestroyBuffer(logicalDevice, indexBuffer, nullptr);
	vkFreeMemory(logicalDevice, indexBufferMemory, nullptr);
}

void Mesh::createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices)
{
	// Get size of buffer needed for vertices
	VkDeviceSize bufferSize = sizeof(Vertex) * vertices->size();

	// Temporary buffer to "stage" vertex data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	// Create Staging Buffer and Allocate Memory to it
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&stagingBuffer, &stagingBufferMemory);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;																// 1. Create pointer to a point in normal memory
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);			// 2. "Map" the vertex buffer memory to that point
	memcpy(data, vertices->data(), (size_t)bufferSize);							// 3. Copy memory from vertices vector to the point
	vkUnmapMemory(logicalDevice, stagingBufferMemory);									// 4. Unmap the vertex buffer memory

	// Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
	// Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not CPU (host)
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);

	// Copy staging buffer to vertex buffer on GPU
	copyBuffer(logicalDevice, transferQueue, transferCmdPool, stagingBuffer, vertexBuffer, bufferSize);

	// Clean up staging buffer parts
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<uint32_t>* indices)
{
	VkDeviceSize bufferSize = sizeof(uint32_t) * indices->size();

	// Temporary buffer to "stage" index data before transferring to GPU
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	// MAP MEMORY TO VERTEX BUFFER
	void* data;																
	vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);		
	memcpy(data, indices->data(), (size_t)bufferSize);							
	vkUnmapMemory(logicalDevice, stagingBufferMemory);					

	// create bffer for INDEX data opn GPU acess only
	createBuffer(physicalDevice, logicalDevice, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);

	//copy from stagin to gpu access buffer
	copyBuffer(logicalDevice, transferQueue, transferCmdPool, stagingBuffer, indexBuffer, bufferSize);

	//clean staging buffer res
	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
	vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
}
