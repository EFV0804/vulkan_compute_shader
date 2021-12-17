#include "VkRenderer.h"
#include <set>
#include <iostream>

VkRenderer::VkRenderer()
{
}

VkRenderer::~VkRenderer()
{
}

int VkRenderer::init(GLFWwindow* pWindow)
{
    window = pWindow;
    try
    {
        createInstance();
        createSurface();
        getPhysicalDevice();
        getQueueFamilyIndices();
        createDevice();
        createQueues();
    }
    catch (const std::runtime_error& e)
    {
        printf("ERROR: %s\n", e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;

    
}

void VkRenderer::createInstance()
{
    vk::ApplicationInfo AppInfo{
    "Super Compute Shader",
    1,
    nullptr,
    0,
    VK_API_VERSION_1_1
    };

    const vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" }; // VK_LAYER_KHRONOS_validation = debug help

    vector<const char*> instanceExtensions;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (size_t i = 0; i < glfwExtensionCount; ++i)
    {
        instanceExtensions.push_back(glfwExtensions[i]);
    }

    if (!checkInstanceExtensionSupport(instanceExtensions))
    {
        throw std::runtime_error("VkInstance does not support required extensions, which is a shame, isn't it?");
    }

    vk::InstanceCreateInfo instanceCreateInfo(vk::InstanceCreateFlags(),
        &AppInfo,
        layers.size(),
        layers.data());

    instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

    instance = vk::createInstance(instanceCreateInfo);
}

void VkRenderer::createSurface()
{
    VkSurfaceKHR surface_temp;
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface_temp);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a vulkan surface.");
    }
    surface = surface_temp; //c++ wrapper type conversion: vk::SurfaceKHR contains VkSurfaceKHR adress
}

SwapchainDetails VkRenderer::getSwapchainDetails()
{
    SwapchainDetails swapchainDetails;
    swapchainDetails.surfaceCapabilities = mainDevices.physicalDevice.getSurfaceCapabilitiesKHR(surface);
    swapchainDetails.formats = mainDevices.physicalDevice.getSurfaceFormatsKHR(surface);
    swapchainDetails.presentationModes = mainDevices.physicalDevice.getSurfacePresentModesKHR(surface);
    return swapchainDetails;
}

bool VkRenderer::checkInstanceExtensionSupport(const vector<const char*>& checkExtensions)
{
    vector<vk::ExtensionProperties> extensions = vk::enumerateInstanceExtensionProperties();

    for (const auto& checkExtension : checkExtensions)
    {
        bool hasExtension = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(checkExtension, extension.extensionName) == 0)
            {
                hasExtension = true;
                break;
            }
        }
        if (!hasExtension) return false;
    }
    return true;
}

bool VkRenderer::checkDeviceExtensionSupport()
{
    vector<vk::ExtensionProperties> extensions = mainDevices.physicalDevice.enumerateDeviceExtensionProperties(nullptr);

    for (auto deviceextension : deviceExtensions)
    {
        bool hasextension = false;
        for (const auto& extension : extensions)
        {
            if (strcmp(deviceextension, extension.extensionName) == 0)
            {
                hasextension = true;
                break;
            }
        }
        if (!hasextension) return false;
    }
    return true;
}

bool VkRenderer::checkDeviceSuitable(vk::PhysicalDevice physicalDevice)
{
    vk::PhysicalDeviceProperties physicalDeviceProperties = physicalDevice.getProperties();
    vk::PhysicalDeviceFeatures physicalDeviceFeatures = physicalDevice.getFeatures();
    
    bool extensionsSupported = checkDeviceExtensionSupport();

    bool swapchainValid = false;

    if (extensionsSupported)
    {
        SwapchainDetails swapchainDetails = getSwapchainDetails();
        swapchainValid = !swapchainDetails.presentationModes.empty() && !swapchainDetails.formats.empty();
    }

    return queueFamilyIndices.isValid() && extensionsSupported && swapchainValid;
}

void VkRenderer::getPhysicalDevice()
{
    vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    if (physicalDevices.size() == 0)
    {
        throw std::runtime_error("Can't find any GPU that supports vulkan, and what are you gonna do about it?");
    }
    for (const auto& device : physicalDevices)
    {
        mainDevices.physicalDevice = device;

        if (checkDeviceSuitable(device))
        {
            mainDevices.physicalDevice = device;
            break;
        }
    }

}

void VkRenderer::createDevice()
{
    float queuePriority = 1.0f;
    uint32_t numQueues = 2;
    vector<vk::DeviceQueueCreateInfo> queuesCreateInfos;

    
    std::set<uint32_t> indices = { queueFamilyIndices.computeFamily, queueFamilyIndices.graphicsFamily };

    for (auto index : indices) {
        vk::DeviceQueueCreateInfo deviceComputeQueueCreateInfo{};
        deviceComputeQueueCreateInfo.sType = vk::StructureType::eDeviceQueueCreateInfo;
        deviceComputeQueueCreateInfo.flags = vk::DeviceQueueCreateFlags();
        deviceComputeQueueCreateInfo.pNext = nullptr;
        deviceComputeQueueCreateInfo.queueCount = 1;
        deviceComputeQueueCreateInfo.queueFamilyIndex = index;
        deviceComputeQueueCreateInfo.pQueuePriorities = &queuePriority;
        queuesCreateInfos.push_back(deviceComputeQueueCreateInfo);
    }
    vk::PhysicalDeviceFeatures deviceFeatures{};
    vk::DeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.flags = vk::DeviceCreateFlags();
    deviceCreateInfo.queueCreateInfoCount = queuesCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queuesCreateInfos.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();


    mainDevices.device = mainDevices.physicalDevice.createDevice(deviceCreateInfo);
}

void VkRenderer::createQueues()
{
    graphicsQueue = mainDevices.device.getQueue(queueFamilyIndices.graphicsFamily, 0);
    computeQueue = mainDevices.device.getQueue(queueFamilyIndices.computeFamily, 0);
    presentationQueue = mainDevices.device.getQueue(queueFamilyIndices.presentationFamily, 0);
}

void VkRenderer::getQueueFamilyIndices()
{

    vector<vk::QueueFamilyProperties> queueFamilyProperties = mainDevices.physicalDevice.getQueueFamilyProperties();

    auto computePropertiesIterator = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [&](const vk::QueueFamilyProperties& properties)
        {
            return properties.queueFlags & requestedQueueFlags.eCompute;
        });

    queueFamilyIndices.computeFamily = std::distance(queueFamilyProperties.begin(), computePropertiesIterator);

    auto graphicsPropertiesIterator = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [&](const vk::QueueFamilyProperties& properties)
        {
            return properties.queueFlags & requestedQueueFlags.eGraphics;
        });

    uint32_t temp_index = std::distance(queueFamilyProperties.begin(), graphicsPropertiesIterator);
    vk::Bool32 presentationSupport = false;
    presentationSupport = mainDevices.physicalDevice.getSurfaceSupportKHR(temp_index, surface);
    if (temp_index >= 0 && presentationSupport)
    {
        queueFamilyIndices.presentationFamily = temp_index;
        queueFamilyIndices.graphicsFamily = temp_index;
    }



    queueFamilyIndices.graphicsFamily = std::distance(queueFamilyProperties.begin(), graphicsPropertiesIterator);

}

vk::ShaderModule VkRenderer::createShader(std::vector<char> shaderCode)
{
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{};
    shaderModuleCreateInfo.sType = vk::StructureType::eShaderModuleCreateInfo;
    shaderModuleCreateInfo.codeSize = shaderCode.size();
    shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

    vk::ShaderModule shaderModule = mainDevices.device.createShaderModule(shaderModuleCreateInfo);
    return shaderModule;
}

void VkRenderer::cleanUp()
{
    instance.destroySurfaceKHR(surface);
    mainDevices.device.destroy();
    instance.destroy();
}