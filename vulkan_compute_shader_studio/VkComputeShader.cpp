#include "VkComputeShader.h"
#include "VkRenderer.h"
#include "VkUtilities.h"
#include <iostream>

VkComputeShader::VkComputeShader(const char* pFileName): fileName { pFileName }
{
}

VkComputeShader::~VkComputeShader()
{
}

void VkComputeShader::load_compute_shader(VkRenderer* renderer)
{
    vector<char> shaderContent = readShaderFile(fileName);
    shaderModule = renderer->createShader(shaderContent);
}

void VkComputeShader::cleanUp(VkRenderer* renderer)
{
    renderer->mainDevices.device.destroyShaderModule(shaderModule);
}