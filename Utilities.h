#pragma once
#include <fstream>
const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indices (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndices {
	int graphicsFamily = -1;			// Location of Graphics Queue Family
	int presentationFamily = -1;		// Location of Presentation Queue Family

	// Check if queue families are valid
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails 
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;		// Surface properties, e.g. image size/extent
	std::vector<VkSurfaceFormatKHR> formats;			// Surface image formats, e.g. RGBA and size of each colour
	std::vector<VkPresentModeKHR> presentationModes;	// How images should be presented to screen
};
struct SwapChainImage
{
	VkImage image;
	VkImage imageView;
};
static std::vector<char> readFile(const std::string& fileName)
{
	//std:binary tells to read file as binary
	//std::ate tells to start reading from END (At The End)
	std::ifstream file(fileName, std::ios::binary | std::iostream::ate);

	//Check if file stream success
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> fileBuffer(fileSize);
	file.seekg(0);								// Move read position back to 0 (at the start)
	file.read(fileBuffer.data(), fileSize);
	file.close();
	return fileBuffer;
}