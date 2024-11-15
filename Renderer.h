#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Utilities.h"
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <string>

class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();
private:
	GLFWwindow* window;
	VkInstance instance;

	struct
	{
		VkPhysicalDevice physical;
		VkDevice logical;
	} device;

	VkQueue graphicsQueue;

	//Vulkan Create Functions
	void CreateInstance();
	void CreateLogicalDevice();

	//Vulkan Support Functions
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool CheckDeviceSuitable(VkPhysicalDevice device);
	std::unordered_set<std::string> GetInstanceExtensions();

	//Get Functions
	void GetPhysicalDevice();
	QueueFamilyIndices GetQueueFamilies(VkPhysicalDevice device);


};

