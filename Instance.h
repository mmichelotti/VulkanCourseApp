#pragma once
#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"
#include <stdexcept>
#include <vector>
#include <unordered_set>
#include <string>

class Instance
{
public:
	Instance();
	VkInstance GetInstance() { return instance; }
	~Instance();
private:
	VkInstance instance;
	bool CheckExtensionSupport(std::vector<const char*>* checkExtensions);
	std::unordered_set<std::string> GetExtensions();
};

