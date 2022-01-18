#pragma once
#include "VkRenderer.h"
//class Vertex;

class VkGraphics
{
public:
	VkGraphics(VkRenderer* pRenderer);
	~VkGraphics();

	void init(vk::Buffer vertexBuffer, uint32_t stride, size_t offsetPos,
		size_t offsetCol, uint32_t verticesSize);
	void clean();
	void draw();

private:
	VkRenderer* renderer;
	vk::SwapchainKHR swapchain;
	vk::Format swapchainImageFormat;
	vk::Extent2D swapchainExtent;
	vector<SwapchainImage> swapchainImages;
	vk::PipelineLayout pipelineLayout;
	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;
	vector<vk::Framebuffer> swapchainFramebuffers;
	vk::CommandPool graphicsCommandPool;
	vector<vk::CommandBuffer> commandBuffers;
	vector<vk::Semaphore> imageAvailable;
	vector<vk::Semaphore> renderFinished;
	const int MAX_FRAME_DRAWS = 2;
	int currentFrame = 0;
	vector<vk::Fence> drawFences;

	void createSwapchain();
	vk::SurfaceFormatKHR chooseBestSurfaceFormat(const vector<vk::SurfaceFormatKHR>& formats);
	vk::PresentModeKHR chooseBestPresentationMode(const vector<vk::PresentModeKHR>& presentationModes);
	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities);
	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
	void createGraphicsPipeline(uint32_t stride, size_t offsetPos, size_t offsetCol);
	void createRenderPass();
	void createFramebuffers();
	void createGraphicsCommandPool();
	void createGraphicsCommandBuffer();

	void recordCommands(vk::Buffer outBuffer, uint32_t verticesSize);
	void createSynchronisation();
};

