#include "./renderer.h"
#include "../../engine.h"

#include <stb/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

uint32_t renderer::currentFrame = 0;
bool renderer::framebufferResized = false;

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
VkDescriptorSetLayout renderer::descriptorSetLayout;
VkPipelineLayout renderer::pipelineLayout;
VkPipeline renderer::graphicsPipeline;

VkCommandPool renderer::commandPool;
std::vector<VkCommandBuffer> renderer::commandBuffers;

VkDescriptorPool renderer::descriptorPool;
std::vector<VkDescriptorSet> renderer::descriptorSets;

VkImage renderer::textureImage;
VkDeviceMemory renderer::textureImageMemory;
VkImageView renderer::textureImageView;
VkSampler renderer::textureSampler;

VkImage renderer::depthImage;
VkDeviceMemory renderer::depthImageMemory;
VkImageView renderer::depthImageView;

std::vector<VkSemaphore> renderer::imageAvailableSemaphores;
std::vector<VkSemaphore> renderer::renderFinishedSemaphores;
std::vector<VkFence> renderer::inFlightFences;

const std::string modelPath = "assets/house/groundPlane.obj";
const std::string texturePath = "assets/house/groundPlane.png";


std::vector<renderer::vertex> renderer::vertices;
std::vector<uint32_t> renderer::indices;

VkBuffer renderer::vertexBuffer;
VkDeviceMemory renderer::vertexBufferMemory;
VkBuffer renderer::indexBuffer;
VkDeviceMemory renderer::indexBufferMemory;
std::vector<VkBuffer> renderer::uniformBuffers;
std::vector<VkDeviceMemory> renderer::uniformBuffersMemory;
std::vector<void*> renderer::uniformBuffersMapped;

#ifdef NDEBUG
	const bool renderer::validationLayersEnabled = false;
#else
	const bool renderer::validationLayersEnabled = true;
#endif

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
	renderer::createDescriptorSetLayout();
	renderer::createGraphicsPipeline();
	renderer::createCommandPool();
	renderer::createDepthResources();
	renderer::createFramebuffers();
	renderer::createTextureImage();
	renderer::createTextureImageView();
	renderer::createTextureSampler();
	renderer::loadModel();
	renderer::createVertexBuffer();
	renderer::createIndexBuffer();
	renderer::createUniformBuffers();
	renderer::createDescriptorPool();
	renderer::createDescriptorSets();
	renderer::createCommandBuffers();
	renderer::createSyncObjects();
}

void renderer::mainLoop() {
	renderer::drawFrame();
}

void renderer::loadModel() {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, modelPath.c_str())) {
		throw std::runtime_error("Failed to load model!");
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			renderer::vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };


			renderer::vertices.push_back(vertex);
			renderer::indices.push_back(indices.size());
		}
	}
}

void renderer::drawFrame() {
	vkWaitForFences(renderer::device, 1, &renderer::inFlightFences[renderer::currentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(renderer::device, renderer::swapChain, UINT64_MAX, renderer::imageAvailableSemaphores[renderer::currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
		renderer::recreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to acquire swapchain image!");
	}

	vkResetFences(renderer::device, 1, &renderer::inFlightFences[renderer::currentFrame]);

	vkResetCommandBuffer(renderer::commandBuffers[renderer::currentFrame], 0);
	renderer::recordCommandBuffer(renderer::commandBuffers[renderer::currentFrame], imageIndex);

	renderer::updateUniformBuffer(renderer::currentFrame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {renderer::imageAvailableSemaphores[renderer::currentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;

	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &renderer::commandBuffers[renderer::currentFrame];

	VkSemaphore signalSemaphores[] = {renderer::renderFinishedSemaphores[renderer::currentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;
	
	if (vkQueueSubmit(renderer::graphicsQueue, 1, &submitInfo, renderer::inFlightFences[renderer::currentFrame]) != VK_SUCCESS) {
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

	renderer::currentFrame = (renderer::currentFrame + 1) % renderer::maxFramesInFlight;
}

void renderer::updateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	uniformBufferObject ubo{};
	ubo.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
	ubo.view = camera::getView();
	ubo.proj = glm::perspective(camera::getFOV(), renderer::swapChainExtent.width / (float)renderer::swapChainExtent.height, 0.1f, 100.0f);
	ubo.proj[1][1] *= -1;

	memcpy(renderer::uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void renderer::recreateSwapChain() {
	logger::log("Recreating swapchain...", 4);
	
	int width = 0, height = 0;

	SDL_Vulkan_GetDrawableSize(engine::window, &width, &height);
	while (width == 0 || height == 0) {
		SDL_Vulkan_GetDrawableSize(engine::window, &width, &height);
		SDL_WaitEvent(&engine::event);
	}

	vkDeviceWaitIdle(renderer::device);

	renderer::cleanupSwapChain();

	renderer::createSwapChain();
	renderer::createImageViews();
	renderer::createDepthResources();
	renderer::createFramebuffers();

	logger::log("Successfully recreated swapchain!", 1);
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
	queueFamilyIndices indices = renderer::findQueueFamilies(physicalDevice);

	bool extensionsSupported = renderer::checkDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;

	if (extensionsSupported) {
		renderer::swapChainSupportDetails swapChainSupport = renderer::querySwapChainSupport(physicalDevice);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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
		renderer::swapChainImageViews[i] = renderer::createImageView(renderer::swapChainImages[i], renderer::swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
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

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = renderer::findDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;

	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;

	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(renderer::device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass!");
	}
	else {
		logger::log("Successfully created render pass!", 1);
	}
}

void renderer::createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutBinding samplerLayoutBinding{};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	createInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(renderer::device, &createInfo, nullptr, &renderer::descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor set layout!");
	}
	else {
		logger::log("Successfully created descriptor set layout!", 1);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
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

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = renderer::vertex::getBindingDescription();
	auto attributeDescriptions = renderer::vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

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

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;

	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};

	std::vector<VkDynamicState> dynamicStates = {
	VK_DYNAMIC_STATE_VIEWPORT,
	VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

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
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &renderer::descriptorSetLayout;
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
	pipelineInfo.pDepthStencilState = &depthStencil;
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
		std::array<VkImageView, 2> attachments = {
			renderer::swapChainImageViews[i],
			renderer::depthImageView
		};

		VkFramebufferCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderer::renderPass;
		createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		createInfo.pAttachments = attachments.data();
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

uint32_t renderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(renderer::physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			logger::log("Successfully found suitable memory type!", 1);
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}

VkFormat renderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(renderer::physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features) {
			return format;
		}

		throw std::runtime_error("Failed to find support format!");
	}
}

bool renderer::hasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat renderer::findDepthFormat() {
	return renderer::findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void renderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
	VkBufferCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(renderer::device, &createInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(renderer::device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = renderer::findMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(renderer::device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	vkBindBufferMemory(renderer::device, buffer, bufferMemory, 0);

	logger::log("Successfully created buffer!", 1);
}

void renderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = renderer::beginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	renderer::endSingleTimeCommands(commandBuffer);

	logger::log("Successfully copied buffer!", 1);
}

void renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
	VkCommandBuffer commandBuffer = renderer::beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	renderer::endSingleTimeCommands(commandBuffer);
}

void renderer::createVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(renderer::vertices[0]) * renderer::vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(renderer::device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, renderer::vertices.data(), (size_t) bufferSize);
	vkUnmapMemory(renderer::device, stagingBufferMemory);

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderer::vertexBuffer, renderer::vertexBufferMemory);

	renderer::copyBuffer(stagingBuffer, renderer::vertexBuffer, bufferSize);

	vkDestroyBuffer(renderer::device, stagingBuffer, nullptr);
	vkFreeMemory(renderer::device, stagingBufferMemory, nullptr);

	logger::log("Successfully created vertex buffer!", 1);
}

void renderer::createIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(renderer::indices[0]) * renderer::indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(renderer::device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, renderer::indices.data(), (size_t)bufferSize);
	vkUnmapMemory(renderer::device, stagingBufferMemory);

	renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderer::indexBuffer, renderer::indexBufferMemory);

	renderer::copyBuffer(stagingBuffer, renderer::indexBuffer, bufferSize);

	vkDestroyBuffer(renderer::device, stagingBuffer, nullptr);
	vkFreeMemory(renderer::device, stagingBufferMemory, nullptr);

	logger::log("Successfully created index buffer!", 1);
}

void renderer::createUniformBuffers() {
	VkDeviceSize bufferSize = sizeof(renderer::uniformBufferObject);

	renderer::uniformBuffers.resize(renderer::maxFramesInFlight);
	renderer::uniformBuffersMemory.resize(renderer::maxFramesInFlight);
	renderer::uniformBuffersMapped.resize(renderer::maxFramesInFlight);

	for (size_t i = 0; i < renderer::maxFramesInFlight; i++) {
		renderer::createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, renderer::uniformBuffers[i], renderer::uniformBuffersMemory[i]);
		
		vkMapMemory(renderer::device, renderer::uniformBuffersMemory[i], 0, bufferSize, 0, &renderer::uniformBuffersMapped[i]);
	}
}

void renderer::createDescriptorPool() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(renderer::maxFramesInFlight);

	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = static_cast<uint32_t>(renderer::maxFramesInFlight);

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	createInfo.pPoolSizes = poolSizes.data();
	createInfo.maxSets = static_cast<uint32_t>(renderer::maxFramesInFlight);

	if (vkCreateDescriptorPool(renderer::device, &createInfo, nullptr, &renderer::descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create descriptor pool!");
	}
	else {
		logger::log("Successfully created descriptor pool!", 1);
	}
}

void renderer::createDescriptorSets() {
	std::vector<VkDescriptorSetLayout> layouts(renderer::maxFramesInFlight, renderer::descriptorSetLayout);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = renderer::descriptorPool;
	allocateInfo.descriptorSetCount = static_cast<uint32_t>(renderer::maxFramesInFlight);
	allocateInfo.pSetLayouts = layouts.data();

	renderer::descriptorSets.resize(renderer::maxFramesInFlight);

	if (vkAllocateDescriptorSets(renderer::device, &allocateInfo, renderer::descriptorSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate descriptor sets!");
	}
	else {
		logger::log("Successfully allocated descriptor sets!", 1);
	}

	for (size_t i = 0; i < renderer::maxFramesInFlight; i++) {
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = renderer::uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(renderer::uniformBufferObject);

		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = renderer::textureImageView;
		imageInfo.sampler = renderer::textureSampler;

		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = renderer::descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
	
		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = renderer::descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &imageInfo;
	
		vkUpdateDescriptorSets(renderer::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void renderer::createSyncObjects() {
	renderer::imageAvailableSemaphores.resize(renderer::maxFramesInFlight);
	renderer::renderFinishedSemaphores.resize(renderer::maxFramesInFlight);
	renderer::inFlightFences.resize(renderer::maxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < renderer::maxFramesInFlight; i++) {
		if (vkCreateSemaphore(renderer::device, &semaphoreCreateInfo, nullptr, &renderer::imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(renderer::device, &semaphoreCreateInfo, nullptr, &renderer::renderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(renderer::device, &fenceCreateInfo, nullptr, &renderer::inFlightFences[i]) != VK_SUCCESS) {

			throw std::runtime_error("Failed to create semaphores and fence!");
		}
		else {
			logger::log("Successfully created semaphores and fence!", 1);
		}
	}
}

VkCommandBuffer renderer::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = renderer::commandPool;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(renderer::device, &allocateInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void renderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(renderer::graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(renderer::graphicsQueue);

	vkFreeCommandBuffers(renderer::device, renderer::commandPool, 1, &commandBuffer);
}

void renderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.extent.width = static_cast<uint32_t>(width);
	createInfo.extent.height = static_cast<uint32_t>(height);
	createInfo.extent.depth = 1;
	createInfo.mipLevels = 1;
	createInfo.arrayLayers = 1;
	createInfo.format = format;
	createInfo.tiling = tiling;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.usage = usage;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.flags = 0;

	if (vkCreateImage(renderer::device, &createInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture image!");
	}
	else {
		logger::log("Successfully created texture image!", 1);
	}

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(renderer::device, image, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = renderer::findMemoryType(memoryRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(renderer::device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate texture image memory!");
	}
	else {
		logger::log("Successfully allocated texture image memory!", 1);
	}

	vkBindImageMemory(renderer::device, image, imageMemory, 0);
}

VkImageView renderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;

	createInfo.subresourceRange.aspectMask = aspectFlags;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(renderer::device, &createInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture image view!");
	}
	else {
		logger::log("Successfully created texture image view!", 1);
	}

	return imageView;
}

void renderer::createTextureImage() {
	int textureWidth, textureHeight, textureChannels;

	stbi_uc* pixels = stbi_load(texturePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);

	VkDeviceSize imageSize = textureWidth * textureHeight * 4;

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	renderer::createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
	
	void* data;
	vkMapMemory(renderer::device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(renderer::device, stagingBufferMemory);

	stbi_image_free(pixels);

	renderer::createImage(textureWidth, textureHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderer::textureImage, renderer::textureImageMemory);

	renderer::transitionImageLayout(renderer::textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	renderer::copyBufferToImage(stagingBuffer, renderer::textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));
	renderer::transitionImageLayout(renderer::textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(renderer::device, stagingBuffer, nullptr);
	vkFreeMemory(renderer::device, stagingBufferMemory, nullptr);
}

void renderer::createTextureImageView() {
	textureImageView = renderer::createImageView(renderer::textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void renderer::createTextureSampler() {
	VkSamplerCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.minFilter = VK_FILTER_LINEAR;

	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	createInfo.anisotropyEnable = VK_TRUE;

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(renderer::physicalDevice, &properties);

	createInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	createInfo.unnormalizedCoordinates = VK_FALSE;

	createInfo.compareEnable = VK_FALSE;
	createInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.mipLodBias = 0.0f;
	createInfo.minLod = 0.0f;
	createInfo.maxLod = 0.0f;

	if (vkCreateSampler(renderer::device, &createInfo, nullptr, &renderer::textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
	else {
		logger::log("Successfully created texture sampler!", 1);
	}
}

void renderer::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = renderer::beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;

	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (renderer::hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::invalid_argument("Unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	renderer::endSingleTimeCommands(commandBuffer);
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

void renderer::createDepthResources() {
	VkFormat depthFormat = renderer::findDepthFormat();

	renderer::createImage(renderer::swapChainExtent.width, renderer::swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, renderer::depthImage, renderer::depthImageMemory);

	renderer::depthImageView = renderer::createImageView(renderer::depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	renderer::transitionImageLayout(renderer::depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void renderer::createCommandBuffers() {
	renderer::commandBuffers.resize(renderer::maxFramesInFlight);

	VkCommandBufferAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.commandPool = renderer::commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = (uint32_t)renderer::commandBuffers.size();

	if (vkAllocateCommandBuffers(renderer::device, &allocateInfo, renderer::commandBuffers.data()) != VK_SUCCESS) {
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

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer::graphicsPipeline);

	VkBuffer vertexBuffers[] = {renderer::vertexBuffer};
	VkDeviceSize offsets = {0};

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, &offsets);

	vkCmdBindIndexBuffer(commandBuffer, renderer::indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer::pipelineLayout, 0, 1, &renderer::descriptorSets[renderer::currentFrame], 0, nullptr);

	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

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

void renderer::cleanup() {
	logger::log("Cleaning up renderer...", 4);

	vkDeviceWaitIdle(renderer::device);

	renderer::cleanupSwapChain();

	vkDestroySampler(renderer::device, renderer::textureSampler, nullptr);
	vkDestroyImageView(renderer::device, renderer::textureImageView, nullptr);

	vkDestroyImage(renderer::device, renderer::textureImage, nullptr);
	vkFreeMemory(renderer::device, renderer::textureImageMemory, nullptr);

	for (size_t i = 0; i < renderer::maxFramesInFlight; i++) {
		vkDestroyBuffer(renderer::device, renderer::uniformBuffers[i], nullptr);
		vkFreeMemory(renderer::device, renderer::uniformBuffersMemory[i], nullptr);
	}

	vkDestroyDescriptorPool(renderer::device, renderer::descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(renderer::device, renderer::descriptorSetLayout, nullptr);

	vkDestroyBuffer(renderer::device, renderer::vertexBuffer, nullptr);
	vkFreeMemory(renderer::device, renderer::vertexBufferMemory, nullptr);

	vkDestroyBuffer(renderer::device, renderer::indexBuffer, nullptr);
	vkFreeMemory(renderer::device, renderer::indexBufferMemory, nullptr);

	vkDestroyPipeline(renderer::device, renderer::graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(renderer::device, renderer::pipelineLayout, nullptr);

	vkDestroyRenderPass(renderer::device, renderer::renderPass, nullptr);

	for (size_t i = 0; i < renderer::maxFramesInFlight; i++) {
		vkDestroySemaphore(renderer::device, renderer::imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(renderer::device, renderer::renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(renderer::device, renderer::inFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(renderer::device, renderer::commandPool, nullptr);

	vkDestroyDevice(renderer::device, nullptr);

	if (renderer::validationLayersEnabled) {
		logger::log("Cleaning up debug messenger...", 4);

		renderer::destroyDebugUtilsMessengerEXT(renderer::instance, renderer::debugMessenger, nullptr);

		logger::log("Cleaned up debug messenger!", 1);
	}

	vkDestroySurfaceKHR(renderer::instance, renderer::surface, nullptr);
	vkDestroyInstance(renderer::instance, nullptr);

	logger::log("Cleaned up renderer!", 1);
}

void renderer::cleanupSwapChain() {
	logger::log("Cleaning up swapchain...", 4);

	vkDeviceWaitIdle(renderer::device);

	vkDestroyImageView(renderer::device, renderer::depthImageView, nullptr);
	vkDestroyImage(renderer::device, renderer::depthImage, nullptr);
	vkFreeMemory(renderer::device, renderer::depthImageMemory, nullptr);

	for (size_t i = 0; i < renderer::swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(renderer::device, renderer::swapChainFramebuffers[i], nullptr);
	}

	for (size_t i = 0; i < renderer::swapChainImageViews.size(); i++) {
		vkDestroyImageView(renderer::device, renderer::swapChainImageViews[i], nullptr);
	}

	vkDestroySwapchainKHR(renderer::device, renderer::swapChain, nullptr);

	logger::log("Cleaned up swapchain!", 1);
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
