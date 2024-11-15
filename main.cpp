#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Window.h"
#include "Renderer.h"

#include <stdexcept>
#include <vector>

#include <iostream>

Window* window = new Window("Test");
Renderer* renderer = new Renderer(window->GetWindow());

int main()
{
	while (!window->IsRunning())
	{
		glfwPollEvents();
	}
	delete window;
	delete renderer;
	return 0;
}