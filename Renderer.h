#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Instance.h"
class Renderer
{
public:
	Renderer(GLFWwindow* window);
	~Renderer();
private:
	GLFWwindow* window;
	Instance* instance;
	Device* device;

};