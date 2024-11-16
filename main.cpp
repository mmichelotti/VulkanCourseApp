#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Window.h"
#include "Renderer.h"

#include <stdexcept>
#include <vector>

#include <iostream>

Window window = Window("Test Window");
Renderer renderer = Renderer(window);

int main()
{
	while (!window.IsRunning())
	{
		glfwPollEvents();
	}
	return 0;
}