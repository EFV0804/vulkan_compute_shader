#include <vulkan/vulkan.hpp>
#include <vector>
#include <iostream>
#include <ostream>
#include <fstream>
#include <algorithm>
#include <iterator>


using std::vector;
using std::cout;
using std::endl;

int main() {

    //APPLICATION
    vk::ApplicationInfo AppInfo{
        "Super Compute Shader",
        1,
        nullptr,
        0,
        VK_API_VERSION_1_1
    };

    //INSTANCE
    const vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" }; // VK_LAYER_KHRONOS_validation = debug help
    vk::InstanceCreateInfo InstanceCreateInfo(vk::InstanceCreateFlags(),
        &AppInfo,
        layers.size(),
        layers.data());
    vk::Instance instance = vk::createInstance(InstanceCreateInfo);


    //PHYSICAL DEVICE
    vk::PhysicalDevice physicalDevice = instance.enumeratePhysicalDevices().front();
    vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    cout << "Device name : " << deviceProperties.deviceName << endl;
    const uint32_t apiVersion = deviceProperties.apiVersion;
    cout << "Vulkan version : " << VK_VERSION_MAJOR(apiVersion) << "." << VK_VERSION_MINOR(apiVersion) << "." << VK_VERSION_PATCH(apiVersion) << endl;
    vk::PhysicalDeviceLimits deviceLimits = deviceProperties.limits;
    uint32_t sizeLimit = deviceLimits.maxComputeSharedMemorySize / 1024;
    cout << "Max compute shared memory size : " << sizeLimit << "KB" << endl;

    //QUEUE
    vector<vk::QueueFamilyProperties> queueFamilyProperties = physicalDevice.getQueueFamilyProperties();

    auto propertiesIterator = std::find_if(queueFamilyProperties.begin(), queueFamilyProperties.end(), [](const vk::QueueFamilyProperties& properties)
        {
            return properties.queueFlags & vk::QueueFlagBits::eCompute;
        });

    const uint32_t computeFamilyQueueIndex = std::distance(queueFamilyProperties.begin(), propertiesIterator);
    cout << "Compute Queue Family Index : " << computeFamilyQueueIndex << endl;

    //create queue instance
    float queuePriority = 1.0f;

    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(),
        computeFamilyQueueIndex,
        1,
        &queuePriority);


    //DEVICE
    vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(),
        deviceQueueCreateInfo);
    vk::Device device = physicalDevice.createDevice(deviceCreateInfo);


    //BUFFERS
    const uint32_t numElements = 10;
    const uint32_t bufferSize = numElements * sizeof(int32_t);

    //create info used in buffer creation
    vk::BufferCreateInfo bufferCreateInfo{
        vk::BufferCreateFlags(),
        bufferSize,
        vk::BufferUsageFlagBits::eStorageBuffer,
        vk::SharingMode::eExclusive,
        1,
        &computeFamilyQueueIndex
    };

    //buffer creation
    vk::Buffer inBuffer = device.createBuffer(bufferCreateInfo);
    vk::Buffer outBuffer = device.createBuffer(bufferCreateInfo);

    //query device for size, alignment and memory type bits for our buffers
    vk::MemoryRequirements inBufferMemoryRequirements = device.getBufferMemoryRequirements(inBuffer);
    vk::MemoryRequirements outBufferMemoryRequirements = device.getBufferMemoryRequirements(outBuffer);

    vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

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

    cout << "Memory Type Index: " << memoryTypeIndex << endl;
    cout << "Memory Heap Size: " << memoryHeapSize / 1024 / 1024 / 1024 << "GB" << endl;

    //Device allocating required memory to buffers
    vk::MemoryAllocateInfo inBufferMemoryAllocateInfo(inBufferMemoryRequirements.size, memoryTypeIndex);
    vk::MemoryAllocateInfo outBufferMemoryAllocataInfo(outBufferMemoryRequirements.size, memoryTypeIndex);
    vk::DeviceMemory inBufferMemory = device.allocateMemory(inBufferMemoryAllocateInfo);
    vk::DeviceMemory outBufferMemory = device.allocateMemory(outBufferMemoryAllocataInfo);

    // make pointer to allocated memory on CPU
    int32_t* inBufferPtr = static_cast<int32_t*>(device.mapMemory(inBufferMemory, 0, bufferSize));

    //Populate buffer
    for (int32_t i = 0; i < numElements; ++i) {
        inBufferPtr[i] = i;
    }

    device.unmapMemory(inBufferMemory);

    //binding buffers to memory
    device.bindBufferMemory(inBuffer, inBufferMemory, 0);
    device.bindBufferMemory(outBuffer, outBufferMemory, 0);

    //CREATE SHADER MODULE
    std::ifstream file{ "compiled_shader.spv", std::ios::binary | std::ios::ate };
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open a file");
    }
    size_t fileSize = (size_t)file.tellg();
    vector<char> shaderContent(fileSize);
    file.seekg(0);
    file.read(shaderContent.data(), fileSize);
    file.close();

    //create shader module from stored SPIRV content above
    vk::ShaderModuleCreateInfo shaderModuleInfo(
        vk::ShaderModuleCreateFlags(),
        shaderContent.size(),
        reinterpret_cast<const uint32_t*>(shaderContent.data()));

    vk::ShaderModule shaderModule = device.createShaderModule(shaderModuleInfo);

    //DESCRIPTOR SET LAYOUT
    //bindings
    const std::vector<vk::DescriptorSetLayoutBinding> DescriptorSetLayoutBinding = {
        {0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
        {1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute} };

    //DescriptorSet layout info
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutInfo(
        vk::DescriptorSetLayoutCreateFlags(),
        DescriptorSetLayoutBinding);
    vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutInfo);

    //PIPELINE
    //Pipeline layout
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo(
        vk::PipelineLayoutCreateFlags(),
        descriptorSetLayout
    );
    vk::PipelineLayout pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
    vk::PipelineCache pipelineCache = device.createPipelineCache(vk::PipelineCacheCreateInfo());


    //pipeline
    vk::PipelineShaderStageCreateInfo pipelineShaderInfo(
        vk::PipelineShaderStageCreateFlags(),
        vk::ShaderStageFlagBits::eCompute,
        shaderModule,
        "main");

    vk::ComputePipelineCreateInfo computePipelineInfo(
        vk::PipelineCreateFlags(),
        pipelineShaderInfo,
        pipelineLayout);
    vk::Pipeline computePipeline = device.createComputePipeline(pipelineCache, computePipelineInfo).value; //.value to avoid implicit conversion to vk::ResultValue https://github.com/KhronosGroup/Vulkan-Hpp#return-values-error-codes--exceptions

    //DESCRIPTOR SET
    //Descriptor pool
    vk::DescriptorPoolSize descriptorPoolSize(vk::DescriptorType::eStorageBuffer, 2);
    vk::DescriptorPoolCreateInfo DescriptorPoolInfo(vk::DescriptorPoolCreateFlags(), 1, descriptorPoolSize);
    vk::DescriptorPool descriptorPool = device.createDescriptorPool(DescriptorPoolInfo);

    //Allocate descriptor sets
    vk::DescriptorSetAllocateInfo descriptorAllocateInfo(descriptorPool, 1, &descriptorSetLayout);
    const std::vector<vk::DescriptorSet> descriptorSets = device.allocateDescriptorSets(descriptorAllocateInfo);
    vk::DescriptorSet descriptorSet = descriptorSets.front();
    vk::DescriptorBufferInfo inBufferInfo(inBuffer, 0, numElements * sizeof(int32_t));
    vk::DescriptorBufferInfo outBufferInfo(outBuffer, 0, numElements * sizeof(int32_t));

    const std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
        {descriptorSet, 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &inBufferInfo},
        {descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &outBufferInfo}
    };
    device.updateDescriptorSets(writeDescriptorSets, {});


    //SUBMIT WORK
    //command poool
    vk::CommandPoolCreateInfo commandPoolInfo(vk::CommandPoolCreateFlags(), computeFamilyQueueIndex);
    vk::CommandPool commandPool = device.createCommandPool(commandPoolInfo);

    //command buffer
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        1);

    const std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(commandBufferAllocateInfo);
    vk::CommandBuffer commandBuffer = commandBuffers.front();

    //recording commands, dispatch
    //Here compute shader is run as many times as their are elements written in buffer
    vk::CommandBufferBeginInfo commandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    commandBuffer.begin(commandBufferBeginInfo);
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, computePipeline);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
        pipelineLayout,
        0,
        { descriptorSet },
        {});
    commandBuffer.dispatch(numElements, 1, 1);
    commandBuffer.end();

    //submit commandBuffer to queue
    vk::Queue queue = device.getQueue(computeFamilyQueueIndex, 0);
    vk::Fence fence = device.createFence(vk::FenceCreateInfo());
    vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, &commandBuffer);
    queue.submit({ submitInfo }, fence);
    device.waitForFences({ fence }, true, uint64_t(-1));

    //map output buffer and cout results
    inBufferPtr = static_cast<int32_t*>(device.mapMemory(inBufferMemory, 0, bufferSize));
    int32_t* outBufferPtr = static_cast<int32_t*>(device.mapMemory(outBufferMemory, 0, bufferSize));

    for (uint32_t i = 0; i < numElements; ++i) {
        std::cout << inBufferPtr[i] << " ";
    }
    std::cout << std::endl;

    device.unmapMemory(inBufferMemory);

    for (uint32_t i = 0; i < numElements; ++i) {
        std::cout << outBufferPtr[i] << " ";
    }
    std::cout << std::endl;
    device.unmapMemory(outBufferMemory);

    //CLEANUP
    device.resetCommandPool(commandPool);
    device.destroyFence(fence);
    device.destroyDescriptorSetLayout(descriptorSetLayout);
    device.destroyPipelineLayout(pipelineLayout);
    device.destroyPipelineCache(pipelineCache);
    device.destroyShaderModule(shaderModule);
    device.destroyPipeline(computePipeline);
    device.destroyDescriptorPool(descriptorPool);
    device.destroyCommandPool(commandPool);
    device.destroyBuffer(inBuffer);
    device.destroyBuffer(outBuffer);
    device.freeMemory(inBufferMemory);
    device.freeMemory(outBufferMemory);
    device.destroy();
    instance.destroy();

    return 0;
}
