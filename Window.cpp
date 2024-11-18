#include "Window.h"

Window::Window(std::string name, unsigned int width, unsigned int height)
{
	glfwInit();
	//set glfw to not work with oppengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
}

bool Window::IsRunning() 
{
	return !glfwWindowShouldClose(window);
}

Window::~Window()
{
	glfwDestroyWindow(window);
	glfwTerminate();
}
