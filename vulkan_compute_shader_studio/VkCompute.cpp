#include "VkCompute.h"

VkCompute::VkCompute()
{
}

VkCompute::VkCompute(VkRenderer* pRenderer, const char* pFileName): renderer{pRenderer}, shaderFileName{pFileName}
{

}

VkCompute::~VkCompute()
{
}

void VkCompute::init(vk::DescriptorBufferInfo inBufferInfo, vk::DescriptorBufferInfo outBufferInfo)
{
	computeShader.load_compute_shader(renderer);
	createDescriptorSetLayout();
	createComputePipeline();
	createDescriptorSet(inBufferInfo, outBufferInfo);
	createCommandBuffer();
}

void VkCompute::run(uint32_t num_elements)
{
	recordCommands( num_elements);
	submitWork();
}

void VkCompute::clean()
{
	renderer->mainDevices.device.resetCommandPool(commandPool);
	renderer->mainDevices.device.destroyDescriptorSetLayout(descriptorSetLayout);
	renderer->mainDevices.device.destroyPipelineLayout(pipelineLayout);
	computeShader.cleanUp(renderer);
	renderer->mainDevices.device.destroyPipeline(computePipeline);
	renderer->mainDevices.device.destroyDescriptorPool(descriptorPool);
	renderer->mainDevices.device.destroyCommandPool(commandPool);

}

void VkCompute::createDescriptorSetLayout()
{

	const std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding = {
		{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute} };

	vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo(
		vk::DescriptorSetLayoutCreateFlags(),
		DescriptorSetLayoutBinding);
	descriptorSetLayout = renderer->mainDevices.device.createDescriptorSetLayout(descriptorSetLayoutInfo);

}

void VkCompute::createComputePipeline()
{
	vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
		vk::PipelineLayoutCreateFlags(),
		descriptorSetLayout
	);
	pipelineLayout = renderer->mainDevices.device.createPipelineLayout(pipelineLayoutCreateInfo);
	vk::PipelineCache pipelineCache = renderer->mainDevices.device.createPipelineCache(vk::PipelineCacheCreateInfo());



	vk::PipelineShaderStageCreateInfo pipelineShaderInfo(
		vk::PipelineShaderStageCreateFlags(),
		vk::ShaderStageFlagBits::eCompute,
		computeShader.shaderModule,
		"main");

	vk::ComputePipelineCreateInfo computePipelineInfo(
		vk::PipelineCreateFlags(),
		pipelineShaderInfo,
		pipelineLayout);
	computePipeline = renderer->mainDevices.device.createComputePipeline(pipelineCache, computePipelineInfo).value;
	renderer->mainDevices.device.destroyPipelineCache(pipelineCache);
}

void VkCompute::createDescriptorSet(vk::DescriptorBufferInfo inBufferInfo, vk::DescriptorBufferInfo outBufferInfo)
{
	vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2);
	vk::DescriptorPoolCreateInfo DescriptorPoolInfo(vk::DescriptorPoolCreateFlags(), 1, descriptorPoolSize);
	descriptorPool = renderer->mainDevices.device.createDescriptorPool(DescriptorPoolInfo);

	vk::DescriptorSetAllocateInfo descriptorAllocateInfo(descriptorPool, 1, &descriptorSetLayout);
	const std::vector<vk::DescriptorSet> descriptorSets = renderer->mainDevices.device.allocateDescriptorSets(descriptorAllocateInfo);
	descriptorSet = descriptorSets.front();



	const std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
		{descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &inBufferInfo},
		{descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &outBufferInfo}
	};
	renderer->mainDevices.device.updateDescriptorSets(writeDescriptorSets, {});

}

void VkCompute::createCommandBuffer()
{
	uint32_t index = renderer->queueFamilyIndices.computeFamily;
	vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), index);
	commandPoolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	commandPool = renderer->mainDevices.device.createCommandPool(commandPoolInfo);

	vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
		commandPool,
		vk::CommandBufferLevel::ePrimary,
		1);

	const std::vector<vk::CommandBuffer> commandBuffers = renderer->mainDevices.device.allocateCommandBuffers(commandBufferAllocateInfo);
	commandBuffer = commandBuffers.front();


}

void VkCompute::recordCommands(uint32_t num_elements)
{
	vk::CommandBufferBeginInfo commandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	commandBuffer.begin(commandBufferBeginInfo);
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
	commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
		pipelineLayout,
		0,
		{ descriptorSet },
		{});
	commandBuffer.dispatch(num_elements, 1, 1);
	commandBuffer.end();
}

void VkCompute::submitWork()
{
	vk::Queue* queuePtr = &renderer->computeQueue;
	vk::Fence fence = renderer->mainDevices.device.createFence(vk::FenceCreateInfo());
	vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer);
	queuePtr->submit({ submitInfo }, fence);
	renderer->mainDevices.device.waitForFences({ fence }, true, uint64_t(-1));
	renderer->mainDevices.device.destroyFence(fence);
}