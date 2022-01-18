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

	//inBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(inBufferMemory, 0, bufferSize));
	//outBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(outBufferMemory, 0, bufferSize));

	//for (uint32_t i = 0; i < numElements; ++i) {
	//	std::cout << inBufferPtr[i] << " ";
	//}
	//std::cout << std::endl;
	//renderer->mainDevices.device.unmapMemory(inBufferMemory);

	//for (uint32_t i = 0; i < numElements; ++i) {
	//	std::cout << outBufferPtr[i] << " ";
	//}
	//std::cout << std::endl;
	//renderer->mainDevices.device.unmapMemory(outBufferMemory);



	graphics.draw();
	updateBuffers(); //breaks
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
	
	auto inBufferPtr = static_cast<float*>(renderer->mainDevices.device.mapMemory(inBufferMemory, 0,bufferSize));

	//struct MyStruct
	//{
	//	glm::vec3 testVec = { 0,1,2 };

	//} testStruct;

	//memcpy(inBufferPtr, &testStruct.testVec, sizeof(testStruct.testVec));



	//std::cout << sizeof(testStruct) << std::endl;
	//std::cout << sizeof(testStruct.testVec) << std::endl;
	//std::cout << sizeof(testStruct.testVec[0]) << std::endl;
	//std::cout << sizeof(uint32_t) << std::endl;

	//std::cout << "Struct adress " << &testStruct << std::endl;
	//std::cout << "Vector adress " << &testStruct.testVec << std::endl;
	//std::cout << "Vector x adress " << &testStruct.testVec[0] << " value = " << &testStruct.testVec[0] << std::endl;
	//std::cout << "Vector y adress " << &testStruct.testVec[1] << " value = " << &testStruct.testVec[1] << std::endl;
	//std::cout << "Vector z adress " << &testStruct.testVec[2] << " value = " << &testStruct.testVec[2] << std::endl;
	//std::cout << "buffer adress " << inBufferPtr << " value = " << *inBufferPtr << std::endl;

	//for (uint32_t i = 0; i < 3; ++i) {
	//	std::cout << inBufferPtr[i] << " " << "adress is " << &inBufferPtr[i]<< std::endl;

	//}

	memcpy(inBufferPtr, vertices.data(), bufferSize);

	std::cout << "Vertices size is " << sizeof(vertices) << std::endl;
	std::cout << "Vertex size is " << sizeof(Vertex) << std::endl;
	std::cout << "GLM vec3 size is "<< sizeof(glm::vec3) << std::endl;
	std::cout << "Buffer size is " << bufferSize << std::endl;

	for (uint32_t i = 0; i < 32; ++i) {
		std::cout << inBufferPtr[i] << " ";
	}

	//for (uint32_t i = 0; i < numElements; ++i) {


	//	std::cout << "element " << i << std::endl;

	//	for (uint32_t j = 0; j < 2; ++j) {

	//		std::cout << "vector " << j << std::endl;
	//		for (uint32_t k = 0; k < 3; ++k) {

	//			std::cout << inBufferPtr[k] << " ";
	//		}
	//		std::cout << std::endl;
	//	}
	//	std::cout << std::endl;
	//}

	std::cout << std::endl;
	renderer->mainDevices.device.unmapMemory(inBufferMemory);


	auto vertexBufferPtr = static_cast<float*>(renderer->mainDevices.device.mapMemory(vertexBufferMemory, 0, bufferSize));
	memcpy(vertexBufferPtr, inBufferPtr, bufferSize);
	for (uint32_t i = 0; i < 32; ++i) {
		std::cout << vertexBufferPtr[i] << " ";
	}
	std::cout << std::endl;
	renderer->mainDevices.device.unmapMemory(vertexBufferMemory);

}

vk::DescriptorBufferInfo Simulation::getDescriptorBufferInfo(vk::Buffer buffer)
{
	vk::DescriptorBufferInfo bufferInfo(buffer, 0, numElements * sizeof(Vertex));
	return bufferInfo;
}

void Simulation::updateBuffers()
{
	auto inBufferPtr = static_cast<float*>(renderer->mainDevices.device.mapMemory(inBufferMemory, 0, bufferSize));
	auto outBufferPtr = static_cast<float*>(renderer->mainDevices.device.mapMemory(outBufferMemory, 0, bufferSize));
	vertexBufferPtr = static_cast<int32_t*>(renderer->mainDevices.device.mapMemory(vertexBufferMemory, 0, bufferSize));

	std::cout << "outBuffer current " ;

	for (uint32_t i = 0; i < 32; ++i) {
		std::cout << outBufferPtr[i] << " ";
	}
	std::cout << std::endl;
	memcpy(inBufferPtr, outBufferPtr, bufferSize);
	memcpy(vertexBufferPtr, inBufferPtr, bufferSize);

	renderer->mainDevices.device.unmapMemory(inBufferMemory);
	renderer->mainDevices.device.unmapMemory(outBufferMemory);
	renderer->mainDevices.device.unmapMemory(vertexBufferMemory);
}
