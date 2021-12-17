#pragma once
#include "VkRenderer.h"
#include "VkGraphics.h"
#include "VkCompute.h"
#include <glm/glm.hpp>

class Simulation
{
public:
	Simulation(VkRenderer* pRenderer, const char* pFileName);
	~Simulation();

	void init();
	void run();
	void close();

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
	};

	const uint32_t numElements = 3;

	const std::vector<Vertex> vertices = {
	{{0.0, -0.4, 0.0}, {1.0, 0.0, 0.0}},
	{{0.4, 0.4, 0.0}, {0.0, 1.0, 0.0}},
	{{-0.4, 0.4, 0.0}, {0.0, 0.0, 1.0}}
	};

private:

	const char* shaderFileName;
	VkRenderer* renderer;
	VkCompute compute{ renderer, shaderFileName };
	VkGraphics graphics{ renderer};

	const uint32_t bufferSize = numElements * sizeof(Vertex);

	vk::Buffer inBuffer;
	vk::DeviceMemory inBufferMemory;
	int32_t* inBufferPtr = nullptr;
	vk::Buffer outBuffer;
	vk::DeviceMemory outBufferMemory;
	int32_t* outBufferPtr = nullptr;

	vk::Buffer vertexBuffer;
	vk::DeviceMemory vertexBufferMemory;
	int32_t* vertexBufferPtr = nullptr;

	//init
	void createBuffer();
	void allocateBufferMemory();
	void bindBuffers();
	void populateInBuffer();
	vk::DescriptorBufferInfo getDescriptorBufferInfo(vk::Buffer buffer);
	void updateBuffers();
	void printOut();
};

