#include "Simulation.h"
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;


Simulation::Simulation(VkRenderer* pRenderer, const char* pShaderFileName) :renderer{ pRenderer },
shaderFileName{ pShaderFileName }
{

}

Simulation::~Simulation()
{
}

void Simulation::init()
{

	createBuffer();
	allocateBufferMemory();
	bindBuffers();
	populateInBuffer();

	vk::DescriptorBufferInfo inBufferInfo = getDescriptorBufferInfo(inBuffer);
	vk::DescriptorBufferInfo outBufferInfo = getDescriptorBufferInfo(outBuffer);
	compute.init(inBufferInfo, outBufferInfo);

	uint32_t stride = sizeof(Vertex);
	size_t offsetPos = offsetof(Vertex, pos);
	size_t offsetCol = offsetof(Vertex, color);
	uint32_t verticesSize = static_cast<uint32_t>(vertices.size());

	graphics.init(vertexBuffer, stride, offsetPos, offsetCol, verticesSize);


}

void Simulation::run()
{
	compute.run(numElements);
	graphics.draw();
	//updateBuffers(); //breaks
}

void Simulation::close()
{
	
	renderer->mainDevices.device.destroyBuffer(inBuffer);
	renderer->mainDevices.device.destroyBuffer(outBuffer);

	renderer->mainDevices.device.freeMemory(inBufferMemory);
	renderer->mainDevices.device.freeMemory(outBufferMemory);

	compute.clean();
	graphics.clean();
	renderer->mainDevices.device.destroyBuffer(vertexBuffer);
	renderer->mainDevices.device.freeMemory(vertexBufferMemory);
	renderer->cleanUp();

}

void Simulation::createBuffer()
{
	vk::BufferCreateInfo inBufferCreateInfo{
		vk::BufferCreateFlags(),
		bufferSize,
		vk::BufferUsageFlagBits::eStorageBuffer,
		vk::SharingMode::eExclusive,
		1,
		&renderer->queueFamilyIndices.computeFamily
	};

	vk::BufferCreateInfo outBufferCreateInfo{
	vk::BufferCreateFlags(),
	bufferSize,
	vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eVertexBuffer,
	vk::SharingMode::eExclusive,
	1,
	& renderer->queueFamilyIndices.computeFamily
	};

	vk::BufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sType = vk::StructureType::eBufferCreateInfo;
	vertexBufferInfo.size = bufferSize;
	vertexBufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	vertexBufferInfo.sharingMode = vk::SharingMode::eExclusive;

	vertexBuffer = renderer->mainDevices.device.createBuffer(vertexBufferInfo);
	inBuffer = renderer->mainDevices.device.createBuffer(inBufferCreateInfo);
	outBuffer = renderer->mainDevices.device.createBuffer(outBufferCreateInfo);

}

void Simulation::allocateBufferMemory()
{
	vk::MemoryRequirements inBufferMemoryRequirements = renderer->mainDevices.device.getBufferMemoryRequirements(inBuffer);
	vk::MemoryRequirements outBufferMemoryRequirements = renderer->mainDevices.device.getBufferMemoryRequirements(outBuffer);
	vk::MemoryRequirements vertexBufferMemoryRequirements = renderer->mainDevices.device.getBufferMemoryRequirements(vertexBuffer);

	vk::PhysicalDeviceMemoryProperties memoryProperties = renderer->mainDevices.physicalDevice.getMemoryProperties();

	uint32_t memoryTypeIndex = uint32_t(~0);
	vk::DeviceSize memoryHeapSize = uint32_t(~0);

	for (uint32_t currentMemoryTypeIndex = 0; currentMemoryTypeIndex < memoryProperties.memoryTypeCount; ++currentMemoryTypeIndex) {
		vk::MemoryType memoryType = memoryProperties.memoryTypes[currentMemoryTypeIndex];
		if ((vk::MemoryPropertyFlagBits::eHostVisible & memoryType.propertyFlags) && (vk::MemoryPropertyFlagBits::eHostCoherent & memoryType.propertyFlags)) {
			memoryHeapSize = memoryProperties.memoryHeaps[memoryType.heapIndex].size;
			memoryTypeIndex = currentMemoryTypeIndex;
			break;
		}
	}

	vk::MemoryAllocateInfo inBufferMemoryAllocateInfo(inBufferMemoryRequirements.size, memoryTypeIndex);
	vk::MemoryAllocateInfo outBufferMemoryAllocataInfo(outBufferMemoryRequirements.size, memoryTypeIndex);
	vk::MemoryAllocateInfo vertexBufferMemoryAllocataInfo(outBufferMemoryRequirements.size, memoryTypeIndex);

	inBufferMemory = renderer->mainDevices.device.allocateMemory(inBufferMemoryAllocateInfo);
	outBufferMemory = renderer->mainDevices.device.allocateMemory(outBufferMemoryAllocataInfo);
	vertexBufferMemory = renderer->mainDevices.device.allocateMemory(vertexBufferMemoryAllocataInfo);
}

void Simulation::bindBuffers()
{
	renderer->mainDevices.device.bindBufferMemory(inBuffer, inBufferMemory, 0);
	renderer->mainDevices.device.bindBufferMemory(outBuffer, outBufferMemory, 0);
	renderer->mainDevices.device.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);
}

void Simulation::populateInBuffer()
{
	
	inBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(inBufferMemory, 0,bufferSize));

	memcpy(inBufferPtr, vertices.data(), bufferSize);
	renderer->mainDevices.device.unmapMemory(inBufferMemory);


	vertexBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(vertexBufferMemory, 0, bufferSize));
	memcpy(vertexBufferPtr, inBufferPtr, bufferSize);
	renderer->mainDevices.device.unmapMemory(vertexBufferMemory);

}

vk::DescriptorBufferInfo Simulation::getDescriptorBufferInfo(vk::Buffer buffer)
{
	vk::DescriptorBufferInfo bufferInfo(buffer, 0, numElements * sizeof(Vertex));
	return bufferInfo;
}

void Simulation::updateBuffers()
{
	inBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(inBufferMemory, 0, bufferSize));
	outBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(outBufferMemory, 0, bufferSize));
	vertexBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(vertexBufferMemory, 0, bufferSize));

	memcpy(inBufferPtr, outBufferPtr, bufferSize);
	memcpy(vertexBufferPtr, inBufferPtr, bufferSize);

	renderer->mainDevices.device.unmapMemory(inBufferMemory);
	renderer->mainDevices.device.unmapMemory(outBufferMemory);
	renderer->mainDevices.device.unmapMemory(vertexBufferMemory);
}
