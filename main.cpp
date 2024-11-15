#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Window.h"

#include <stdexcept>
#include <vector>

#include <iostream>


int main()
{
	Window* window = new Window("Test");
	while (!window->IsRunning())
	{
		glfwPollEvents();
	}
	//delete window;
	return 0;
}