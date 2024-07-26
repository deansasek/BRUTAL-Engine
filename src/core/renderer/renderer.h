#pragma once

#ifndef renderer_h
#define renderer_h

#define NOMINMAX
#define STB_IMAGE_IMPLEMENTATION

#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR

#include <sdl2/include/SDL_vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../filesystem/filesystem.h"
#include "../modules/camera.h"

#include <vector>
#include <set>
#include <optional>
#include <cstdint>
#include <limits>
#include <array>
#include <algorithm>
#include <chrono>

namespace renderer {
	struct vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(vertex, texCoord);

			return attributeDescriptions;
		}
	};

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

	//gebbs

	extern std::vector<vertex> vertices;
	extern std::vector<uint32_t> indices;

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

	extern VkImage textureImage;
	extern VkDeviceMemory textureImageMemory;
	extern VkImageView textureImageView;
	extern VkSampler textureSampler;

	extern VkImage depthImage;
	extern VkDeviceMemory depthImageMemory;
	extern VkImageView depthImageView;

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
	void createDepthResources();
	void createTextureImage();
	void createTextureImageView();
	void createTextureSampler();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	void loadModel();

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void mainLoop();
	void drawFrame();
	void recreateSwapChain();
	void updateUniformBuffer(uint32_t currentImage);

	void cleanup();
	void cleanupSwapChain();

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();
	bool hasStencilComponent(VkFormat format);

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