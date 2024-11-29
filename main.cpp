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

	float angle = 0.0f;
	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	while (mainWindow.IsRunning())
	{
		glfwPollEvents();

		float now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		angle += 10.0f * deltaTime;

		if (angle > 360.0f)
		{
			angle -= 360.0f;
		}
		vulkanRenderer.updateModel(glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 0.0f, 1.0f)));
		vulkanRenderer.draw();
	}

	return EXIT_SUCCESS;
}