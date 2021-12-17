#pragma once
#include "VkRenderer.h"
#include "VkComputeShader.h"


class VkCompute
{
public:
	VkCompute();
	VkCompute(VkRenderer* pRenderer, const char* pFileName);
	~VkCompute();

	void init(vk::DescriptorBufferInfo inBufferInfo, vk::DescriptorBufferInfo outBufferInfo);
	void run(uint32_t num_elements);
	void clean();

private:
	VkRenderer* renderer;
	const char* shaderFileName;
	VkComputeShader computeShader{ shaderFileName };

	vk::DescriptorSetLayout descriptorSetLayout;
	vk::PipelineLayout pipelineLayout;
	vk::DescriptorPool descriptorPool;
	vk::DescriptorSet descriptorSet;
	vk::Pipeline computePipeline;
	vk::CommandPool commandPool;
	vk::CommandBuffer commandBuffer;

	void createDescriptorSetLayout();
	void createComputePipeline();
	void createDescriptorSet(vk::DescriptorBufferInfo inBufferInfo, vk::DescriptorBufferInfo outBufferInfo);
	void createCommandBuffer();

	void recordCommands(uint32_t num_elements);
	void submitWork();
};

