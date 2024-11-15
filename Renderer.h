#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
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

	//Vulkan components
	VkInstance instance;

	//Vulkan Create Functions
	void CreateInstance();

	//Vulkan Support Functions
	bool CheckInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	std::unordered_set<std::string> GetInstanceExtensions();

};

