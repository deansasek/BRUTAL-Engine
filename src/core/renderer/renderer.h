#pragma once

#ifndef renderer_h
#define renderer_h

#define VK_USE_PLATFORM_WIN32_KHR
#define NOMINMAX
#define GLM_FORCE_RADIANS

#include <vulkan/vulkan.h>

#include <sdl2/include/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../filesystem/filesystem.h"

#include <vector>
#include <set>
#include <optional>
#include <cstdint>
#include <limits>
#include <array>
#include <algorithm>
#include <chrono>

namespace renderer {
	const int maxFramesInFlight = 2;
	extern uint32_t currentFrame;
	extern bool framebufferResized;

	extern VkSurfaceKHR surface;
	extern VkInstance instance;

	extern VkPhysicalDevice physicalDevice;
	extern VkDevice device;
	extern VkPhysicalDeviceProperties physicalDeviceProperties;
	extern VkPhysicalDeviceFeatures physicalDeviceFeatures;

	extern VkQueue graphicsQueue;
	extern VkQueue presentQueue;

	struct queueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() const {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct swapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities = {};
		std::vector<VkSurfaceFormatKHR> formats = {};
		std::vector<VkPresentModeKHR> presentModes = {};
	};

	struct uniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	extern VkBuffer vertexBuffer;
	extern VkDeviceMemory vertexBufferMemory;
	extern VkBuffer indexBuffer;
	extern VkDeviceMemory indexBufferMemory;
	extern std::vector<VkBuffer> uniformBuffers;
	extern std::vector<VkDeviceMemory> uniformBuffersMemory;
	extern std::vector<void*> uniformBuffersMapped;

	extern VkSwapchainKHR swapChain;
	extern std::vector<VkImage> swapChainImages;
	extern VkFormat swapChainImageFormat;
	extern VkExtent2D swapChainExtent;
	extern std::vector<VkImageView> swapChainImageViews;
	extern std::vector<VkFramebuffer> swapChainFramebuffers;

	extern VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	extern VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	extern VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	extern VkShaderModule createShaderModule(const std::vector<char>& code);
	extern VkShaderModule vertexShaderModule;
	extern VkShaderModule fragmentShaderModule;

	extern VkRenderPass renderPass;
	extern VkDescriptorSetLayout descriptorSetLayout;
	extern VkPipelineLayout pipelineLayout;
	extern VkPipeline graphicsPipeline;

	extern VkCommandPool commandPool;
	extern std::vector<VkCommandBuffer> commandBuffers;

	extern VkDescriptorPool descriptorPool;
	extern std::vector<VkDescriptorSet> descriptorSets;

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	extern std::vector<VkSemaphore> imageAvailableSemaphores;
	extern std::vector<VkSemaphore> renderFinishedSemaphores;
	extern std::vector<VkFence> inFlightFences;

	void init();
	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void mainLoop();
	void drawFrame();
	void recreateSwapChain();
	void updateUniformBuffer(uint32_t currentImage);

	void cleanup();
	void cleanupSwapChain();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	bool physicalDeviceSuitable(VkPhysicalDevice physicalDevice);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	queueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);

	swapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	// validation layers stuff
	void createDebugMessenger();

	extern VkDebugUtilsMessengerEXT debugMessenger;
	VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	void destroyDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};

	extern bool checkValidationLayerSupport();
	extern const bool validationLayersEnabled;

	std::vector<const char*> getRequiredExtensions();
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);
}

#endif