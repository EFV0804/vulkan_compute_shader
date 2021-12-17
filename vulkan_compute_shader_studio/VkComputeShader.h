#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>

using std::vector;

class VkRenderer;

class VkComputeShader
{
public:
	VkComputeShader(const char* pFileName);
	~VkComputeShader();
	void load_compute_shader(VkRenderer* renderer);
	vk::ShaderModule shaderModule;
	void cleanUp(VkRenderer* renderer);

private:
	const char* fileName;

};

