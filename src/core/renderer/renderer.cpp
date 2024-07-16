#include "./renderer.h"
#include "../../engine.h"

VkInstance renderer::instance;
VkSurfaceKHR renderer::surface;

VkPhysicalDevice renderer::physicalDevice;
VkDevice renderer::device;
VkPhysicalDeviceProperties renderer::physicalDeviceProperties;
VkPhysicalDeviceFeatures renderer::physicalDeviceFeatures;

VkQueue renderer::graphicsQueue;
VkQueue renderer::presentQueue;

VkSwapchainKHR renderer::swapChain;
std::vector<VkImage> renderer::swapChainImages;
VkFormat renderer::swapChainImageFormat;
VkExtent2D renderer::swapChainExtent;
std::vector<VkImageView> renderer::swapChainImageViews;
std::vector<VkFramebuffer> renderer::swapChainFramebuffers;

VkShaderModule renderer::vertexShaderModule;
VkShaderModule renderer::fragmentShaderModule;

VkRenderPass renderer::renderPass;
VkPipelineLayout renderer::pipelineLayout;
VkPipeline renderer::graphicsPipeline;

VkCommandPool renderer::commandPool;
VkCommandBuffer renderer::commandBuffer;

VkSemaphore renderer::imageAvailableSemaphore;
VkSemaphore renderer::renderFinishedSemaphore;
VkFence renderer::inFlightFence;

//#ifdef NDEBUG
	bool renderer::validationLayersEnabled = true;
//#else
//	bool renderer::validationLayersEnabled = false;
//#endif

void renderer::init() {
	logger::log("Initializing renderer...", 4);

	renderer::createInstance();
	renderer::createSurface();
	renderer::createDebugMessenger();
	renderer::pickPhysicalDevice();
	renderer::createLogicalDevice();
	renderer::createSwapChain();
	renderer::createImageViews();
	renderer::createRenderPass();
	renderer::createGraphicsPipeline();
	renderer::createFramebuffers();
	renderer::createCommandPool();
	renderer::createCommandBuffer();
	renderer::createSyncObjects();
}

void renderer::mainLoop() {
	renderer::drawFrame();
}

void renderer::drawFrame() {
	vkWaitForFences(renderer::device, 1, &renderer::inFlightFence, VK_TRUE, UINT64_MAX);
	vkResetFences(renderer::device, 1, &renderer::inFlightFence);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(renderer::device, renderer::swapChain, UINT64_MAX, renderer::imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(renderer::commandBuffer, 0);
	renderer::recordCommandBuffer(renderer::commandBuffer, imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {renderer::imageAvailableSemaphore};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;

	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &renderer::commandBuffer;

	VkSemaphore signalSemaphores[] = {renderer::renderFinishedSemaphore};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(renderer::graphicsQueue, 1, &submitInfo, renderer::inFlightFence) != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {renderer::swapChain};
	
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	vkQueuePresentKHR(renderer::presentQueue, &presentInfo);
}

void renderer::createSurface() {
	if (SDL_Vulkan_CreateSurface(engine::window, renderer::instance, &renderer::surface) == SDL_FALSE) {
		logger::log("Failed to create SDL surface!", 3);
	}
	else {
		logger::log("Successfully created SDL surface!", 1);
	}
}

void renderer::createInstance() {
	if (renderer::validationLayersEnabled && !renderer::checkValidationLayerSupport()) {
		throw std::runtime_error("Validation layers requested, but not available");
	}

	VkApplicationInfo applicationInfo{};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pApplicationName = engine::name;
	applicationInfo.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	applicationInfo.pEngineName = engine::name;
	applicationInfo.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &applicationInfo;

	auto extensions = renderer::getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (renderer::validationLayersEnabled) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		renderer::populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &renderer::instance) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create instance");
	}
	else {
		logger::log("Successfully created instance!", 1);
	}
}

void renderer::pickPhysicalDevice() {
	uint32_t physicalDeviceCount = 0;

	vkEnumeratePhysicalDevices(renderer::instance, &physicalDeviceCount, nullptr);

	if (physicalDeviceCount == 0) {
		throw std::runtime_error("Failed to find Vulkan compatible physical device");
	}

	std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	vkEnumeratePhysicalDevices(renderer::instance, &physicalDeviceCount, physicalDevices.data());

	for (const auto& physicalDevice : physicalDevices) {
		if (renderer::physicalDeviceSuitable(physicalDevice)) {
			renderer::physicalDevice = physicalDevice;

			logger::log(std::string("Successfully located physical device: ") + physicalDeviceProperties.deviceName, 1);

			break;
		}
	}

	if (renderer::physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find suitable physical device");
	}
}

bool renderer::physicalDeviceSuitable(VkPhysicalDevice physicalDevice) {

	//vkGetPhysicalDeviceProperties(physicalDevice, &renderer::physicalDeviceProperties);
	//vkGetPhysicalDeviceFeatures(physicalDevice, &renderer::physicalDeviceFeatures);

	queueFamilyIndices indices = renderer::findQueueFamilies(physicalDevice);

	bool extensionsSupported = renderer::checkDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;

	if (extensionsSupported) {
		renderer::swapChainSupportDetails swapChainSupport = renderer::querySwapChainSupport(physicalDevice);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

bool renderer::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(renderer::deviceExtensions.begin(), renderer::deviceExtensions.end());
	
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

renderer::queueFamilyIndices renderer::findQueueFamilies(VkPhysicalDevice physicalDevice) {
	renderer::queueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;

			logger::log("Successfully found graphics family support!", 1);
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, renderer::surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;

			logger::log("Successfully found present family support!", 1);
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

void renderer::createLogicalDevice() {
	renderer::queueFamilyIndices indices = renderer::findQueueFamilies(renderer::physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(renderer::deviceExtensions.size());
	deviceCreateInfo.ppEnabledExtensionNames = renderer::deviceExtensions.data();

	if (renderer::validationLayersEnabled) {
		deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(renderer::validationLayers.size());
		deviceCreateInfo.ppEnabledLayerNames = renderer::validationLayers.data();
	}
	else {
		deviceCreateInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(renderer::physicalDevice, &deviceCreateInfo, nullptr, &renderer::device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical device");
	}
	else {
		logger::log("Successfully created logical device!", 1);
	}

	vkGetDeviceQueue(renderer::device, indices.graphicsFamily.value(), 0, &renderer::graphicsQueue);
	vkGetDeviceQueue(renderer::device, indices.presentFamily.value(), 0, &renderer::presentQueue);
}

renderer::swapChainSupportDetails renderer::querySwapChainSupport(VkPhysicalDevice physicalDevice) {
	renderer::swapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, renderer::surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, renderer::surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, renderer::surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, renderer::surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);

		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, renderer::surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

VkSurfaceFormatKHR renderer::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR renderer::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			return availablePresentMode;
		}
	}
	
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D renderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		SDL_Vulkan_GetDrawableSize(engine::window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void renderer::createSwapChain() {
	renderer::swapChainSupportDetails swapChainSupport = renderer::querySwapChainSupport(renderer::physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = renderer::chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = renderer::chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = renderer::chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = renderer::surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	renderer::queueFamilyIndices indices = renderer::findQueueFamilies(renderer::physicalDevice);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 1;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(renderer::device, &createInfo, nullptr, &renderer::swapChain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain!");
	}
	else {
		logger::log("Successfully created swapchain!", 1);
	}

	vkGetSwapchainImagesKHR(renderer::device, renderer::swapChain, &imageCount, nullptr);
	renderer::swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(renderer::device, renderer::swapChain, &imageCount, renderer::swapChainImages.data());

	renderer::swapChainImageFormat = surfaceFormat.format;
	renderer::swapChainExtent = extent;
}

void renderer::createImageViews() {
	renderer::swapChainImageViews.resize(renderer::swapChainImages.size());

	for (size_t i = 0; i < renderer::swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = renderer::swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = renderer::swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(renderer::device, &createInfo, nullptr, &renderer::swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create image views!");
		}
		else {
			logger::log("Successfully created image views!", 1);
		}
	}
}

void renderer::createRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = renderer::swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(renderer::device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}
	else {
		logger::log("Successfully created render pass!", 1);
	}
}

void renderer::createGraphicsPipeline() {
	auto vertexShaderCode = filesystem::readFile("./shaders/vert.spv");
	auto fragmentShaderCode = filesystem::readFile("./shaders/frag.spv");

	renderer::vertexShaderModule = renderer::createShaderModule(vertexShaderCode);
	renderer::fragmentShaderModule = renderer::createShaderModule(fragmentShaderCode);

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = renderer::vertexShaderModule;
	vertexShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = renderer::fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) renderer::swapChainExtent.width;
	viewport.height = (float) renderer::swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = renderer::swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;

	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(renderer::device, &pipelineLayoutInfo, nullptr, &renderer::pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout!");
	}
	else {
		logger::log("Successfully created pipeline layout!", 1);
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;

	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;

	pipelineInfo.layout = renderer::pipelineLayout;
	pipelineInfo.renderPass = renderer::renderPass;

	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(renderer::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &renderer::graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(renderer::device, renderer::vertexShaderModule, nullptr);
	vkDestroyShaderModule(renderer::device, renderer::fragmentShaderModule, nullptr);
}

void renderer::createFramebuffers() {
	renderer::swapChainFramebuffers.resize(renderer::swapChainImageViews.size());

	for (size_t i = 0; i < renderer::swapChainImageViews.size(); i++) {
		VkImageView attachments[] = {
			renderer::swapChainImageViews[i]
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderer::renderPass;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = attachments;
		createInfo.width = renderer::swapChainExtent.width;
		createInfo.height = renderer::swapChainExtent.height;
		createInfo.layers = 1;

		if (vkCreateFramebuffer(renderer::device, &createInfo, nullptr, &renderer::swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create framebuffer!");
		}
		else {
			logger::log("Successfully created framebuffer!", 1);
		}
	}
}

void renderer::createSyncObjects() {
	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	if (vkCreateSemaphore(renderer::device, &semaphoreCreateInfo, nullptr, &renderer::imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(renderer::device, &semaphoreCreateInfo, nullptr, &renderer::renderFinishedSemaphore) != VK_SUCCESS ||
		vkCreateFence(renderer::device, &fenceCreateInfo, nullptr, &renderer::inFlightFence) != VK_SUCCESS) {

		throw std::runtime_error("Failed to create semaphores and fence!");
	}
	else {
		logger::log("Successfully created semaphores and fence!", 1);
	}
}

void renderer::createCommandPool() {
	queueFamilyIndices queueFamilyIndices = renderer::findQueueFamilies(renderer::physicalDevice);

	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	if (vkCreateCommandPool(renderer::device, &createInfo, nullptr, &renderer::commandPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create command pool!");
	}
	else {
		logger::log("Successfully created command pool!", 1);
	}
}

void renderer::createCommandBuffer() {
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = renderer::commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(renderer::device, &allocateInfo, &renderer::commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate command buffers!");
	}
	else {
		logger::log("Successfully allocated command buffers!", 1);
	}
}

void renderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderer::renderPass;
	renderPassInfo.framebuffer = renderer::swapChainFramebuffers[imageIndex];
	
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = renderer::swapChainExtent;

	VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer::graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(renderer::swapChainExtent.width);
	viewport.height = static_cast<float>(renderer::swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = {0, 0};
	scissor.extent = renderer::swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to end recording of command buffer!");
	}
}

VkShaderModule renderer::createShaderModule(const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;

	if (vkCreateShaderModule(renderer::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create shader module!");
	}
	else {
		logger::log("Successfully created shader module!", 1);

		return shaderModule;
	}
}

void renderer::cleanUp() {
	logger::log("Cleaning up renderer...", 4);

	vkDestroySemaphore(renderer::device, renderer::imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(renderer::device, renderer::renderFinishedSemaphore, nullptr);
	vkDestroyFence(renderer::device, renderer::inFlightFence, nullptr);

	vkDestroyCommandPool(renderer::device, renderer::commandPool, nullptr);

	if (renderer::validationLayersEnabled) {
		logger::log("Cleaning up debug messenger...", 4);

		renderer::destroyDebugUtilsMessengerEXT(renderer::instance, renderer::debugMessenger, nullptr);

		logger::log("Cleaned up debug messenger!", 1);
	}

	for (auto framebuffer : renderer::swapChainFramebuffers) {
		vkDestroyFramebuffer(renderer::device, framebuffer, nullptr);
	}

	for (auto imageView : renderer::swapChainImageViews) {
		vkDestroyImageView(renderer::device, imageView, nullptr);
	} 

	vkDestroyPipeline(renderer::device, renderer::graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(renderer::device, renderer::pipelineLayout, nullptr);
	vkDestroyRenderPass(renderer::device, renderer::renderPass, nullptr);

	vkDestroySwapchainKHR(renderer::device, renderer::swapChain, nullptr);
	vkDestroyDevice(renderer::device, nullptr);
	vkDestroySurfaceKHR(renderer::instance, renderer::surface, nullptr);
	vkDestroyInstance(renderer::instance, nullptr);

	logger::log("Cleaned up renderer!", 1);
}

// validation layers

VkDebugUtilsMessengerEXT renderer::debugMessenger;

void renderer::createDebugMessenger() {
	if (!renderer::validationLayersEnabled) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	renderer::populateDebugMessengerCreateInfo(createInfo);

	if (renderer::createDebugUtilsMessengerEXT(renderer::instance, &createInfo, nullptr, &renderer::debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create debug messenger");
	}
	else {
		logger::log("Successfully created debug messenger!", 1);
	}
}

void renderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = renderer::debugCallback;
	createInfo.pUserData = nullptr;
}

bool renderer::checkValidationLayerSupport() {
	logger::log("Checking for validation layer support...", 4);

	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : renderer::validationLayers) {
		logger::log("Validation layers not found!", 2);

		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				logger::log("Validation layers found!", 1);

				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			logger::log("No validation layers found!", 3);

			return false;
		}
	}

	return true;
}

std::vector<const char*> renderer::getRequiredExtensions() {
	uint32_t extensionCount;
	const char** extensionNames;

	SDL_Vulkan_GetInstanceExtensions(engine::window, &extensionCount, nullptr);

	extensionNames = new const char* [extensionCount];

	SDL_Vulkan_GetInstanceExtensions(engine::window, &extensionCount, extensionNames);

	std::vector<const char*> extensions(extensionNames, extensionNames + extensionCount);

	if (renderer::validationLayersEnabled) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL renderer::debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "Validation Layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult renderer::createDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {

	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(renderer::instance, "vkCreateDebugUtilsMessengerEXT");

	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void renderer::destroyDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}
