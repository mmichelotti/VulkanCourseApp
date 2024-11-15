#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include "Window.h"
#include "Renderer.h"

#include <stdexcept>
#include <vector>

#include <iostream>

Window* window = new Window("Test");
Renderer renderer;

int main()
{
	if(renderer.Init(window->GetWindow()) == EXIT_FAILURE) return EXIT_FAILURE;
	while (!window->IsRunning())
	{
		glfwPollEvents();
	}
	//delete window;
	return 0;
}