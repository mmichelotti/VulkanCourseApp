#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Instance.h"
#include "Window.h"
class Renderer
{
public:
	Renderer(Window& window);
	~Renderer();
private:
	Instance* instance;
	Device* device;
	VkSurfaceKHR surface;
};