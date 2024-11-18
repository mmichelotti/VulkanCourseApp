#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VkRenderer.h"
#include "Window.h"

int main()
{
	Window mainWindow = Window("Main Window");
	VkRenderer vulkanRenderer = VkRenderer(mainWindow);

	while (mainWindow.IsRunning())
	{
		glfwPollEvents();
	}

	return 0;
}