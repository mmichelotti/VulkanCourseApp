#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <vector>

class Renderer
{
public:
	Renderer();
	int Init(GLFWwindow* window);
private:
	GLFWwindow* window;

	//Vulkan components
	VkInstance instance;

	//Vulkan functions
	void CreateInstance();
};

