#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <stdexcept>
#include <vector>
#include <array>
#include "VkUtilities.h"




using std::vector;

class VkRenderer
{
public:
	VkRenderer();
	~VkRenderer();

	struct {
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
	} mainDevices;

	struct {
		vk::QueueFlagBits eCompute = vk::QueueFlagBits::eCompute;
		vk::QueueFlagBits eGraphics = vk::QueueFlagBits::eGraphics;
	} requestedQueueFlags;

	struct {
		uint32_t graphicsFamily = -1;
		uint32_t presentationFamily = -1;
		uint32_t  computeFamily = -1;
		bool isValid()
		{
			return graphicsFamily >= 0 && presentationFamily >= 0 && computeFamily >= 0;
		}
	} queueFamilyIndices;

	vk::Queue computeQueue;
	vk::Queue graphicsQueue;
	vk::Queue presentationQueue;
	vk::Instance instance;
	GLFWwindow* window;
	vk::SurfaceKHR surface;

	int init(GLFWwindow* pWindow);
	void draw();
	void cleanUp();
	vk::ShaderModule createShader(std::vector<char> shaderCode);
	SwapchainDetails getSwapchainDetails();

private:

	void createInstance();
	void createSurface();
	bool checkInstanceExtensionSupport(const vector<const char*>& checkExtensions);
	void getPhysicalDevice();
	bool checkDeviceSuitable(vk::PhysicalDevice physicalDevice);
	void createDevice();
	void createQueues();
	void getQueueFamilyIndices();
	bool checkDeviceExtensionSupport();

};

