#pragma once
#include "GLFW/glfw3.h"
#include <iostream>

class Window
{
public:
	Window(std::string name, const unsigned int width = 1920, const unsigned int height = 1080);
	GLFWwindow* GetWindow() const { return window; }
	bool IsRunning();
	~Window();
private:
	GLFWwindow* window;
};

