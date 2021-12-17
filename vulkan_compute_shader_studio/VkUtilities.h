#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>

using std::string;
using std::vector;

const vector<const char*> deviceExtensions
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
struct SwapchainDetails
{
    vk::SurfaceCapabilitiesKHR surfaceCapabilities;
    vector<vk::SurfaceFormatKHR> formats;
    vector<vk::PresentModeKHR> presentationModes;
};

struct SwapchainImage
{
    VkImage image;
    VkImageView imageView;
};

static vector<char> readShaderFile(const string& filename)
{
    // Open shader file
    // spv files are binary data, put the pointer at the end of the file to get its size
    std::ifstream file{ filename, std::ios::binary | std::ios::ate };
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open a file");
    }
    // Buffer preparation
    size_t fileSize = (size_t)file.tellg(); // Get the size through the position of the pointer
    vector<char> fileBuffer(fileSize); // Set file buffer to the file size
    file.seekg(0); // Move in file to start of the file
    // Reading and closing
    file.read(fileBuffer.data(), fileSize);
    file.close();
    return fileBuffer;
}