#include "VkRenderer.h"

VkRenderer::VkRenderer(const Window& window) : window(window.GetWindow())
{
	try 
	{
		createInstance();
		createDebugCallback();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createRenderPass();
		createDescriptorSetLayout();
		createPushConstantRange();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandPool();

		uboVP = UboViewProjection((float)swapChainExtent.width, (float)swapChainExtent.height);
		createMesh();


		createCommandBuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createSynchronization();
	}
	catch (const std::runtime_error& e) {
		printf("ERROR: %s\n", e.what());
	}
}
void VkRenderer::updateModel(size_t modelId, glm::mat4 newModel)
{
	if (modelId >= meshes.size()) return;
	meshes[modelId]->setModel(newModel);
}
VkRenderer::~VkRenderer()
{
	vkDeviceWaitIdle(device.logical);

	vkDestroyDescriptorPool(device.logical, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(device.logical, descriptorSetLayout, nullptr);
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		vkDestroyBuffer(device.logical, vpUniformBuffer[i], nullptr);
		vkFreeMemory(device.logical, vpUniformBufferMemory[i], nullptr);
	}
	for (const Mesh* mesh : meshes)
	{
		delete mesh;
	}
	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
	{
		vkDestroySemaphore(device.logical, renderSemaphores[i], nullptr);
		vkDestroySemaphore(device.logical, imageSemaphores[i], nullptr);
		vkDestroyFence(device.logical, drawFences[i], nullptr);
	}
	vkDestroyCommandPool(device.logical, graphicsCommandPool, nullptr);
	for (auto frameBuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(device.logical, frameBuffer, nullptr);
	}
	vkDestroyPipeline(device.logical, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device.logical, pipelineLayout, nullptr);
	vkDestroyRenderPass(device.logical, renderPass, nullptr);
	for (const SwapChainImage& image : swapChainImages)
	{
		vkDestroyImageView(device.logical, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR(device.logical, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyDevice(device.logical, nullptr);
	if (validationEnabled)
	{
		DestroyDebugReportCallbackEXT(instance, callback, nullptr);
	}
	vkDestroyInstance(instance, nullptr);
}
void VkRenderer::draw()
{
	// 1. Get the next available image to draw to and set something to signal when it's finished (semaphor)
	// 2. Submit cmd buffer to queue for execution, need to wait image availability and signal when it's finished
	// 3. Present image to screen when rendering ready

	// -- STOP FOR FENCES -- 
	// wait for given fence to signal (open) from last draw before continuing CPU code, after that unsignal it (close)
	vkWaitForFences(device.logical, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());		
	vkResetFences(device.logical, 1, &drawFences[currentFrame]);

	// -- GET NEXT IMAGE --
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device.logical, swapchain, std::numeric_limits<uint64_t>::max(), imageSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	recordCommands(imageIndex);
	updateUniformBuffers(imageIndex);

	// -- SUBMIT CMD BUFFER TO RENDER --
	VkPipelineStageFlags waitStages[] =
	{
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;								
	submitInfo.pWaitSemaphores = &imageSemaphores[currentFrame];				
	submitInfo.pWaitDstStageMask = waitStages;													// Stages when to check semaphor, in our case we wait when we reach the color attachment
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];									// cmd buffer to submit, since in the chain we have 1 to 1 relation it's at img index
	submitInfo.signalSemaphoreCount = 1;														// n of semaphores to signal when it's finished
	submitInfo.pSignalSemaphores = &renderSemaphores[currentFrame];								// semaphore signalled when we finished rendering

	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);	 // submit cmd buffer to queue and reopen the fence
	checkResult(result, "Failed to submit cmd buffer to queue");

	// -- PRPESENT RENDERED IMAGE TO SCREEN --
	VkPresentInfoKHR presentImageInfo = {};
	presentImageInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentImageInfo.waitSemaphoreCount = 1;
	presentImageInfo.pWaitSemaphores = &renderSemaphores[currentFrame];
	presentImageInfo.swapchainCount = 1;
	presentImageInfo.pSwapchains = &swapchain;
	presentImageInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(graphicsQueue, &presentImageInfo);
	checkResult(result, "Failed to present image to screen");

	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VkRenderer::createInstance()
{
	if (validationEnabled && !checkValidationLayerSupport())
	{
		throw std::runtime_error("Required Validation Layers not supported!");
	}

	// Information about the application itself
	// Most data here doesn't affect the program and is for developer convenience
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";					// Custom name of the application
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);		// Custom version of the application
	appInfo.pEngineName = "No Engine";							// Custom engine name
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);			// Custom engine version
	appInfo.apiVersion = VK_API_VERSION_1_0;					// The Vulkan Version

	// Creation information for a VkInstance (Vulkan Instance)
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Create list to hold instance extensions
	std::vector<const char*> instanceExtensions = std::vector<const char*>();

	// Set up extensions Instance will use
	uint32_t glfwExtensionCount = 0;				// GLFW may require multiple extensions
	const char** glfwExtensions;					// Extensions passed as array of cstrings, so need pointer (the array) to pointer (the cstring)

	// Get GLFW extensions
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Add GLFW extensions to list of extensions
	for (size_t i = 0; i < glfwExtensionCount; i++)
	{
		instanceExtensions.push_back(glfwExtensions[i]);
	}

	// If validation enabled, add extension to report validation debug info
	if (validationEnabled)
	{
		instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	// Check Instance Extensions supported...
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions!");
	}

	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	if (validationEnabled)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}


	// Create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	checkResult(result, "Failed to create a Vulkan Instance!");
}

void VkRenderer::createDebugCallback()
{
	// Only create callback if validation enabled
	if (!validationEnabled) return;

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;	// Which validation reports should initiate callback
	callbackCreateInfo.pfnCallback = debugCallback;												// Pointer to callback function itself

	// Create debug callback with custom create function
	VkResult result = CreateDebugReportCallbackEXT(instance, &callbackCreateInfo, nullptr, &callback);
	checkResult(result,"Failed to create Debug Callback!");
}

void VkRenderer::createLogicalDevice()
{
	//Get the queue family indices for the chosen Physical Device
	QueueFamilyIndices indices = getQueueFamilies(device.physical);

	// Vector for queue creation information, and set for family indices
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

	// Queues the logical device needs to create and info to do so
	for (int queueFamilyIndex : queueFamilyIndices)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex;						// The index of the family to create a queue from
		queueCreateInfo.queueCount = 1;												// Number of queues to create
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;								// Vulkan needs to know how to handle multiple queues, so decide priority (1 = highest priority)

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Information to create logical device (sometimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());		// Number of Queue Create Infos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();								// List of queue create infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());	// Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();							// List of enabled logical device extensions

	// Physical Device Features the Logical Device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};

	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;			// Physical Device features Logical Device will use

	// Create the logical device for the given physical device
	VkResult result = vkCreateDevice(device.physical, &deviceCreateInfo, nullptr, &device.logical);
	checkResult(result, "Failed to create a Logical Device!");


	// Queues are created at the same time as the device...
	// So we want handle to queues
	// From given logical device, of given Queue Family, of given Queue Index (0 since only one queue), place reference in given VkQueue
	vkGetDeviceQueue(device.logical, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device.logical, indices.presentationFamily, 0, &presentationQueue);
}

void VkRenderer::createSurface()
{
	// Create Surface (creates a surface create info struct, runs the create surface function, returns result)
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	checkResult(result, "Failed to create a surface!");
}

void VkRenderer::createSwapChain()
{
	SwapChainDetails swapChainDetails = getSwapChainDetails(device.physical);

	//Find optimal surface values for our swapp chain
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentationMode = chooseBestPresentationMode(swapChainDetails.presentationModes);
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

	//How many image are in the swap chain? get 1 more than the minimum, to amount triple buffer
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
	//If iamge higher than max, then clamp to max
	//And here it happen again, maxImageCount = 0 doesnt mean there are no maxiamges, but there is UNLIMITED (counter intuitive idk)
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	//Creation information for swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.minImageCount = imageCount;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.clipped = VK_TRUE;

	//Get queue family indices 
	QueueFamilyIndices indices = getQueueFamilies(device.physical);

	//IF graphics and presentation are different then swapp chain must let image be shared between families
	if (indices.graphicsFamily != indices.presentationFamily)
	{
		uint32_t queueFamilyIndices[] =
		{
			(uint32_t)indices.graphicsFamily,
			(uint32_t)indices.presentationFamily
		};
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapChainCreateInfo.queueFamilyIndexCount = 2;
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}
	//if old swap chain been destoryed and this one replaces it, then link old one to quickly hand over responsabilities
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	//Create swapchaing
	VkResult result = vkCreateSwapchainKHR(device.logical, &swapChainCreateInfo, nullptr, &swapchain);
	checkResult(result, "Failed to create swapchain");

	//Chace for later references
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	//Get swap chain images
	uint32_t swapChainImageCount;
	vkGetSwapchainImagesKHR(device.logical, swapchain, &swapChainImageCount, nullptr);
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(device.logical, swapchain, &swapChainImageCount, images.data());
	for (VkImage image : images)
	{
		//Store image handle
		SwapChainImage swapChainImage = {};
		swapChainImage.image = image;
		swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		swapChainImages.push_back(swapChainImage);
	}
}

void VkRenderer::createRenderPass()
{
	// Color attachment of RenderPass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Framebffer data will be stored as an image, but images can be given different data layout
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//Attachment reference uses an attachment index that refers to an index in the attahcment list papssed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;

	//Need to determin when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies;
	//conversion from LAYOUT UNDEFINED to LAYOUT COLOR ATTACHMENT OPTIMAL
	// transition must happpen after...
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	// but mast happen before...
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;


	//Create info for render pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(device.logical, &renderPassInfo, nullptr, &renderPass);
	checkResult(result, "Faield to create a render pass");

}

void VkRenderer::createDescriptorSetLayout()
{
	// UboViewProjection Binding Info
	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
	vpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr;							// For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

	// Create Descriptor Set Layout with given bindings
	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());	// Number of binding infos
	layoutCreateInfo.pBindings = layoutBindings.data();

	// create set layout
	VkResult result = vkCreateDescriptorSetLayout(device.logical, &layoutCreateInfo, nullptr, &descriptorSetLayout);
	checkResult(result, "Failed to create descriptor set layout");
}

void VkRenderer::createPushConstantRange()
{
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(glm::mat4);
}

void VkRenderer::createGraphicsPipeline()
{
	// Build Shader Module to link to Grapphics Pipeline
	VkShaderModule vertexShaderModule = createShaderModule("Shaders/vert.spv");
	VkShaderModule fragShaderModule = createShaderModule("Shaders/frag.spv");
	
	// -- SHADER STAGE  --
	 
	// Vertex stage creation information
	VkPipelineShaderStageCreateInfo vertexInfo = {};
	vertexInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexInfo.module = vertexShaderModule;
	vertexInfo.pName = "main";		//can custom the main name of the shaders to be run,

	// Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragInfo = {};
	fragInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragInfo.module = fragShaderModule;
	fragInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertexInfo, fragInfo};
	
	// -- VERTEX INPUT -- 
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;														// can bind multiple streams of data
	bindingDescription.stride = sizeof(Vertex);											// size of a signel vertex object
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;							// how to move between data after each vertex. there is _INSTANCE too to instance multiple objects that are the same

	
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;
	//position attributes
	attributeDescriptions[0].binding = 0;												// which binding the data is at (in shad.frag layout(binding = 0) is implicit
	attributeDescriptions[0].location = 0;												// layout shader location
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, position);						// where this attribute is defined in the data of a singel vertexù

	//color attributes
	attributeDescriptions[1].binding = 0;												
	attributeDescriptions[1].location = 1;												
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	// -- INPUT ASSEMBLY --
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {};
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;	

	// -- VIEWPORT  -- 
	VkViewport viewportInfo = {};
	viewportInfo.x = 0.0f;
	viewportInfo.y = 0.0f;
	viewportInfo.width = (float)swapChainExtent.width;
	viewportInfo.height = (float)swapChainExtent.height;
	viewportInfo.minDepth = 0.0f;
	viewportInfo.maxDepth = 1.0f;

	// -- SCISSOR  -- 
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = swapChainExtent;
	
	// -- VIEWPORT STATE --
	VkPipelineViewportStateCreateInfo viewportStateInfo = {};
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.viewportCount = 1;
	viewportStateInfo.pViewports = &viewportInfo;
	viewportStateInfo.scissorCount = 1;
	viewportStateInfo.pScissors = &scissor;
	
	// -- DYNAMIC STATES --
	/*
	//Dynamic states to enable (basically to stretch the viewport dynamically
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);		//Dynamic viewpport : can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport)
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);		//Dynamic scissor : can resize in cmd buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor)
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
	dynamicState.pDynamicStates = dynamicStateEnables.data();
	// remember that swappchain extent has not changed
	// so each time we resize we need to destroy current swapchain and pass a new swapchain with images resized, also the depth buffer, or any image used
	*/

	// -- RASTERIZER --
	VkPipelineRasterizationStateCreateInfo rasterizerInfo = {};
	rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerInfo.depthClampEnable = VK_FALSE;						//CChange if fragment beyond near/far pplanes are clipppepd (default) or clamped to plane
	rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;				//idk why would you care about discarding data and skip the rasterization process, not clear for now
	rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;				//VERY INTRESTING, can rasterize only lines for instance to render the wireframe // We need to add GPU feature too if we change this
	rasterizerInfo.lineWidth = 1.0f;
	rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;				//Do not draw the reverse normal
	rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;		//determining if the face of a triangle is front base on clockwise, if we could see the back the points would have rendered anticlowckwise (in vulkan Y is inverted so it's viceversa)
	rasterizerInfo.depthBiasEnable = VK_FALSE;						//Wether to add deppth bias to fragments (for stoppipng shadow achne in the shadow mapping)

	// -- MULTISAMPLING --
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// -- COLOR ATTACHMENTS --
	//(how blending is handled)
	VkPipelineColorBlendAttachmentState colorAttachmentInfo = {};
	colorAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorAttachmentInfo.blendEnable = VK_TRUE;
	colorAttachmentInfo.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorAttachmentInfo.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorAttachmentInfo.colorBlendOp = VK_BLEND_OP_ADD;
	//Summarised: (originColorBlendFactor * newColor) colorBlendOp(destinationColorBlendFactor * oldColor)
	//			  (VK_BLEND_FACTOR_SRC_ALPHA * newColor) + (VK_BLEND_FACTOR_ONE_MINUS_ALPHA * oldColor)
	//			  (newColorAlpha * newColor) + ((1- newColorAlpha) * oldColor)
	colorAttachmentInfo.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorAttachmentInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorAttachmentInfo.alphaBlendOp = VK_BLEND_OP_ADD;
	//Summarised: (1 * newAlpha) + (0 * oldAlpha) = newAlpha

	// -- BLENDING --
	VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorAttachmentInfo;

	// -- PIPELINE LAYOUT --
	VkPipelineLayoutCreateInfo pipelineLayoutInfo= {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

	//Create layout
	VkResult result = vkCreatePipelineLayout(device.logical, &pipelineLayoutInfo, nullptr, &pipelineLayout);
	checkResult(result, "Failed to create pipeline layout");


	// -- DEPTH STENCIL TEST --

	// -- GRAPHICS PIPELINE CREATION --
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.pRasterizationState = &rasterizerInfo;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlendInfo;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.layout = pipelineLayout;							// pippeline layout pipeline should use
	pipelineInfo.renderPass = renderPass;							// render pass description pippeline is compatible with
	pipelineInfo.subpass = 0;										// subppass of render pass to use with pipeline

	//Can create multiple pipelines that can derive from one another 
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;				// existing pippelien to derive from...
	pipelineInfo.basePipelineIndex = -1;							// or index of pipeline being created to derive from ( if created multiple )

	result = vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
	checkResult(result,"Failed to create a pipeline");

	// -- DESTROY --
	vkDestroyShaderModule(device.logical, fragShaderModule, nullptr);
	vkDestroyShaderModule(device.logical, vertexShaderModule, nullptr);
}

void VkRenderer::createFrameBuffers()
{
	swapChainFramebuffers.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		std::array<VkImageView, 1> attachments =
		{
			swapChainImages[i].imageView
		};


		VkFramebufferCreateInfo frameBufferInfo = {};		
		frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferInfo.renderPass = renderPass;											// render pass layout the framebuffei will be used ith
		frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());		
		frameBufferInfo.pAttachments = attachments.data();									// list of attachmetns 1 to 1 with render pass
		frameBufferInfo.width = swapChainExtent.width;										
		frameBufferInfo.height = swapChainExtent.height;
		frameBufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device.logical, &frameBufferInfo, nullptr, &swapChainFramebuffers[i]);
		checkResult(result, "Failed to create a frame buffer");

	}
}

void VkRenderer::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = getQueueFamilies(device.physical);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	// Create a graphics queue family cmd pool
	VkResult result = vkCreateCommandPool(device.logical, &poolInfo, nullptr, &graphicsCommandPool);
	checkResult(result,"Failed to craete a command pool");
}

void VkRenderer::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo commandInfo = {};
	commandInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandInfo.commandPool = graphicsCommandPool;
	commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;		//PRIMARY means executed by a POOL, SECONDARY means executed by another CMD BUFFER PRIMARY

	commandInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
	VkResult result = vkAllocateCommandBuffers(device.logical, &commandInfo, commandBuffers.data());
	checkResult(result, "Failed to create commadn buffers!");
	//doenst need to be destoryed like others since we are not creating, we are allocating to the command pool, when the cmd pool is destoryed, also this is destoyed
}

void VkRenderer::createSynchronization()
{
	imageSemaphores.resize(MAX_FRAME_DRAWS);
	renderSemaphores.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;					// to start a fence as open ( by default is closed )

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) 
	{
		if (
			vkCreateSemaphore(device.logical, &semaphoreInfo, nullptr, &imageSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(device.logical, &semaphoreInfo, nullptr, &renderSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(device.logical, &fenceInfo, nullptr, &drawFences[i]) != VK_SUCCESS
			)
		{
			throw std::runtime_error("Failed to create a semaphor or a fence");
		}

	}
}

void VkRenderer::createMesh()
{
	// vertex data should be overlapping without index data
	//std::vector<Vertex> meshVertices =
	//{
	//	{{ 0.4f, -0.4f, 0.0f}, {1.0f, 0.0f, 0.0f}},		// 0
	//	{{ 0.4f,  0.4f, 0.0f}, {0.0f, 1.0f, 0.0f}},		// 1
	//	{{-0.4f,  0.4f, 0.0f}, {0.0f, 0.0f, 1.0f}},		// 2

	//	{{-0.4f,  0.4f, 0.0f}, {0.0f, 0.0f, 1.0f}},		// 2
	//	{{-0.4f, -0.4f, 0.0f}, {1.0f, 1.0f, 0.0f}},		// 3
	//	{{ 0.4f, -0.4f, 0.0f}, {1.0f, 0.0f, 0.0f}}		// 0
	//};
	//firstMesh = new Mesh(device.physical, device.logical, graphicsQueue, graphicsCommandPool, &meshVertices);
	//delete firstMesh;

	// this vertex data is without overlappping
	std::vector<Vertex> meshVertices1 = {
		{ { -0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
		{ { -0.4, -0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
		{ { 0.4, -0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
		{ { 0.4, 0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
	};

	std::vector<Vertex> meshVertices2 = {
		{ { -0.25, 0.6, 0.0 },{ 1.0f, 0.0f, 0.0f } },	// 0
		{ { -0.25, -0.6, 0.0 },{ 0.0f, 1.0f, 0.0f } },	    // 1
		{ { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } },    // 2
		{ { 0.25, 0.6, 0.0 },{ 1.0f, 1.0f, 0.0f } },   // 3
	};

	// Index Data
	std::vector<uint32_t> meshIndices = {
		0, 1, 2,
		2, 3, 0
	};
	Mesh* firstMesh = new Mesh(device, graphicsQueue, graphicsCommandPool, &meshVertices1, &meshIndices);
	Mesh* secondMesh = new Mesh(device, graphicsQueue, graphicsCommandPool, &meshVertices2, &meshIndices);
	meshes.push_back(firstMesh);
	meshes.push_back(secondMesh);

}

void VkRenderer::createUniformBuffers()
{
	// ViewProjection buffer size
	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);


	// One uniform buffer for each image (and by extension, command buffer)
	vpUniformBuffer.resize(swapChainImages.size());
	vpUniformBufferMemory.resize(swapChainImages.size());

	// Create Uniform buffers
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		createBuffer(device, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);
	}
}

void VkRenderer::createDescriptorPool()
{
	// Type of descriptors + how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)
	// ViewProjection Pool
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

	// List of pool sizes
	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

	// Data to create Descriptor Pool
	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());					// Maximum number of Descriptor Sets that can be created from pool
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());		// Amount of Pool Sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();									// Pool Sizes to create pool with

	// Create Descriptor Pool
	VkResult result = vkCreateDescriptorPool(device.logical, &poolCreateInfo, nullptr, &descriptorPool);
	checkResult(result, "Faield to create descriptor pool");
}

void VkRenderer::createDescriptorSets()
{
	// Resize Descriptor Set list so one for every buffer
	descriptorSets.resize(swapChainImages.size());

	std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), descriptorSetLayout);

	// Descriptor Set Allocation Info
	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descriptorPool;									// Pool to allocate Descriptor Set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());// Number of sets to allocate
	setAllocInfo.pSetLayouts = setLayouts.data();									// Layouts to use to allocate sets (1:1 relationship)

	// Allocate descriptor sets (multiple)
	VkResult result = vkAllocateDescriptorSets(device.logical, &setAllocInfo, descriptorSets.data());
	checkResult(result, "Failed to allocate Descriptor Sets!");

	// Update all of descriptor set buffer bindings
	for (size_t i = 0; i < swapChainImages.size(); i++)
	{
		// VIEW PROJECTION DESCRIPTOR
		// Buffer info and data offset info
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = vpUniformBuffer[i];		// Buffer to get data from
		vpBufferInfo.offset = 0;						// Position of start of data
		vpBufferInfo.range = sizeof(UboViewProjection);				// Size of data

		// Data about connection between binding and buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = descriptorSets[i];								// Descriptor Set to update
		vpSetWrite.dstBinding = 0;											// Binding to update (matches with binding on layout/shader)
		vpSetWrite.dstArrayElement = 0;									// Index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Type of descriptor
		vpSetWrite.descriptorCount = 1;									// Amount to update
		vpSetWrite.pBufferInfo = &vpBufferInfo;							// Information about buffer data to bind

		// List of Descriptor Set Writes
		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

		// Update the descriptor sets with new buffer/binding info
		vkUpdateDescriptorSets(device.logical, static_cast<uint32_t>(setWrites.size()), setWrites.data(),
			0, nullptr);
	}
}

void VkRenderer::updateUniformBuffers(uint32_t imgIndex)
{
	void* data;

	// Copy View Projection data
	size_t bufferSize = sizeof(UboViewProjection);
	vkMapMemory(device.logical, vpUniformBufferMemory[imgIndex], 0, bufferSize, 0, &data);
	memcpy(data, &uboVP, bufferSize);
	vkUnmapMemory(device.logical, vpUniformBufferMemory[imgIndex]);
}

void VkRenderer::recordCommands(uint32_t imageIndex)
{
	//Info about how to begin each cmd buffer
	VkCommandBufferBeginInfo cmdBufferBeginInfo = {};
	cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkClearValue clearValues[] =
	{
		{0.0f, 0.0f, 0.0f, 1.0f}
	};

	//Info about how to begin a render pass, only need for graphical application
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset = { 0,0 };
	renderPassBeginInfo.renderArea.extent = swapChainExtent;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.clearValueCount = 1;


	renderPassBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];
	VkResult result = vkBeginCommandBuffer(commandBuffers[imageIndex], &cmdBufferBeginInfo);
	checkResult(result, "Failed to start recording a command buffer!");

		vkCmdBeginRenderPass(commandBuffers[imageIndex], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			
			vkCmdBindPipeline(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			for(size_t j = 0; j < meshes.size(); j++)
			{
				VkBuffer vertexBuffers[] = { meshes[j]->getVertexBuffer()};
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(commandBuffers[imageIndex], 0, 1, vertexBuffers, offsets);
				vkCmdBindIndexBuffer(commandBuffers[imageIndex], meshes[j]->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
				vkCmdPushConstants(commandBuffers[imageIndex], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &meshes[j]->getModel());

				vkCmdBindDescriptorSets(commandBuffers[imageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[imageIndex], 0, nullptr);		//Bind descriptor sets
				vkCmdDrawIndexed(commandBuffers[imageIndex], meshes[j]->getIndexCount(), 1, 0, 0, 0);
			}

		vkCmdEndRenderPass(commandBuffers[imageIndex]);

	result = vkEndCommandBuffer(commandBuffers[imageIndex]);
	checkResult(result, "Failed to stop recording a command buffer");
}

void VkRenderer::getPhysicalDevice()
{
	// Enumerate Physical devices the vkInstance can access
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// If no devices available, then none support Vulkan!
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that support Vulkan Instance!");
	}

	// Get list of Physical Devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	for (const auto& device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			this->device.physical = device;
			break;
		}
	}

	// Get properties of our new physical device and save the min offset alignemnt info
	VkPhysicalDeviceProperties deviceProperties = {};
	vkGetPhysicalDeviceProperties(device.physical, &deviceProperties);
}

bool VkRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	std::unordered_set<std::string> availableExtensions = GetInstanceExtensions();
	if (availableExtensions.size() == 0) return false;
	for (const char* checkExtension : *checkExtensions)
	{
		if (availableExtensions.find(checkExtension) == availableExtensions.end()) return false;
	}
	return true;
}

bool VkRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get device extension count
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// If no extensions found, return failure
	if (extensionCount == 0)
	{
		return false;
	}

	// Populate list of extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// Check for extension
	for (const auto& deviceExtension : deviceExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(deviceExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}

		if (!hasExtension)
		{
			return false;
		}
	}

	return true;
}

bool VkRenderer::checkValidationLayerSupport()
{
	// Get number of validation layers to create vector of appropriate size
	uint32_t validationLayerCount;
	vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr);

	// Check if no validation layers found AND we want at least 1 layer
	if (validationLayerCount == 0 && validationLayers.size() > 0)
	{
		return false;
	}

	std::vector<VkLayerProperties> availableLayers(validationLayerCount);
	vkEnumerateInstanceLayerProperties(&validationLayerCount, availableLayers.data());

	// Check if given Validation Layer is in list of given Validation Layers
	for (const auto& validationLayer : validationLayers)
	{
		bool hasLayer = false;
		for (const auto& availableLayer : availableLayers)
		{
			if (strcmp(validationLayer, availableLayer.layerName) == 0)
			{
				hasLayer = true;
				break;
			}
		}

		if (!hasLayer)
		{
			return false;
		}
	}

	return true;
}

bool VkRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*
	// Information about the device itself (ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines, etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	QueueFamilyIndices indices = getQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainValid = false;
	if (extensionsSupported)
	{
		SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
	}

	return indices.isValid() && extensionsSupported && swapChainValid;
}


QueueFamilyIndices VkRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	// Get all Queue Family Property info for the given device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Go through each queue family and check if it has at least 1 of the required types of queue
	int i = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		// First check if queue family has at least 1 queue in that family (could have no queues)
		// Queue can be multiple types defined through bitfield. Need to bitwise AND with VK_QUEUE_*_BIT to check if has required type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;		// If queue family is valid, then get index
		}

		// Check if Queue Family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		// Check if queue is presentation type (can be both graphics and presentation)
		if (queueFamily.queueCount > 0 && presentationSupport)
		{
			indices.presentationFamily = i;
		}

		// Check if queue family indices are in a valid state, stop searching if so
		if (indices.isValid())
		{
			break;
		}

		i++;
	}

	return indices;
}

SwapChainDetails VkRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;

	// -- CAPABILITIES --
	// Get the surface capabilities for the given surface on the given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

	// -- FORMATS --
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	// If formats returned, get list of formats
	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	// -- PRESENTATION MODES --
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	// If presentation modes returned, get list of presentation modes
	if (presentationCount != 0)
	{
		swapChainDetails.presentationModes.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
	}

	return swapChainDetails;
}

std::unordered_set<std::string> VkRenderer::GetInstanceExtensions()
{
	//Count the number of extensions
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	//Create of vkextensions based on the amount we have
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	//Save it on a set of strings for faster querying
	std::unordered_set<std::string> set;
	for (const VkExtensionProperties& extension : extensions)
	{
		set.insert(extension.extensionName);
	}
	return set;
}


//best format is subjective
//best format for us : VK_FORMAT_R8G8B8A8_UNORM (VK_FORMAT_B8G8R8A8_UNORM is backup)
//best colorSpae	 : VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
VkSurfaceFormatKHR VkRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	//if only 1 format and undefined, this means all format are availalbe (counterintuitive, idk why)
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	}
	//if restricted, search for optimal
	for (const auto& format : formats)
	{
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
			&& format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
		{
			return format;
		}
	}
	//if cant find optimal, return the first format
	return formats[0];
}

VkPresentModeKHR VkRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	//Look for mailbox presentation, otherwise use default FIFO KHR
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return presentationMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VkRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	//if current extentt is at numeric limits, then extent can vary. Otherwise, it is the size of the window
	//Else means the value can vary, need to set manually
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return surfaceCapabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		//Surface also defines max and min, so make sure within boundaries
		//Clamp for widht and for height

		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

VkImageView VkRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = format;
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	
	//subresources : allow the view to view only a part of an image
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags;
	viewCreateInfo.subresourceRange.baseMipLevel = 0; 
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 1;

	//create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(device.logical, &viewCreateInfo, nullptr, &imageView);
	checkResult(result, "Failed to crate iamge view");

	return imageView;
}

VkShaderModule VkRenderer::createShaderModule(const std::string& fileName)
{
	auto code = readFile(fileName);
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); //to cast type of pointer data it uses REINTERPRET keywoard instead of STATIC

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(device.logical, &shaderModuleCreateInfo, nullptr, &shaderModule);
	checkResult(result,"Failed to create a shader module!");

	return shaderModule;
}