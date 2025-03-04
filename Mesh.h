#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"

//VERTEX BUFFER
//INDEX BUFFER

class Mesh
{
public:
    Mesh(Device device, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<Vertex>* vertices, std::vector<uint32_t>* indices, size_t texId);
    ~Mesh();
    
    void cleanUp();
    void setModel(glm::mat4 model) { this->modelMatrix = model; }

#pragma region getters
    const size_t getTexId()             const { return texId; }
    const glm::mat4& getModel()         const { return modelMatrix; }
    const size_t& getVertexCount()      const { return vertex.count; }
    const VkBuffer& getVertexBuffer()   const { return vertex.buffer; }
    const size_t& getIndexCount()       const { return index.count; }
    const VkBuffer& getIndexBuffer()    const { return index.buffer; }
#pragma endregion

private:
    struct MeshData
    {
        size_t count;
        VkBuffer buffer;
        VkDeviceMemory bufferMemory;

        MeshData() : count(0), buffer(VK_NULL_HANDLE), bufferMemory(VK_NULL_HANDLE) {}

        template<typename T>
        MeshData(Device device, VkQueue transferQueue, VkCommandPool transferCmdPool, std::vector<T>* data, VkBufferUsageFlagBits bufferType) 
        {
            count = data->size();
            // Get size of buffer needed for vertices
            VkDeviceSize bufferSize = sizeof(Vertex) * count;

            // Temporary buffer to "stage" vertex data before transferring to GPU
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;

            // Create Staging Buffer and Allocate Memory to it
            createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &stagingBuffer, &stagingBufferMemory);


            // MAP MEMORY TO VERTEX BUFFER
            void* mappedData;  // 1. Create pointer to a point in normal memory
            vkMapMemory(device.logical, stagingBufferMemory, 0, bufferSize, 0, &mappedData);  // 2. "Map" the vertex buffer memory to that point
            memcpy(mappedData, data->data(), (size_t)bufferSize);  // 3. Copy memory from vertices vector to the point
            vkUnmapMemory(device.logical, stagingBufferMemory);  // 4. Unmap the vertex buffer memory

            // Create buffer with TRANSFER_DST_BIT to mark as recipient of transfer data (also VERTEX_BUFFER)
            // Buffer memory is to be DEVICE_LOCAL_BIT meaning memory is on the GPU and only accessible by it and not CPU (host)
            createBuffer(device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | bufferType,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &buffer, &bufferMemory);

            // Copy staging buffer to vertex buffer on GPU
            copyBuffer(device.logical, transferQueue, transferCmdPool, stagingBuffer, buffer, bufferSize);

            // Clean up staging buffer parts
            vkDestroyBuffer(device.logical, stagingBuffer, nullptr);
            vkFreeMemory(device.logical, stagingBufferMemory, nullptr);
        }
    };

    glm::mat4 modelMatrix;
    size_t texId;

    MeshData vertex;
    MeshData index;

    Device device;
};
