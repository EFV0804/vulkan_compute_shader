#include "VkGraphics.h"

VkGraphics::VkGraphics(VkRenderer* pRenderer): renderer{pRenderer}
{
}

VkGraphics::~VkGraphics()
{
}

void VkGraphics::init(vk::Buffer vertexBuffer, uint32_t stride, size_t offsetPos,
    size_t offsetCol, uint32_t verticesSize)
{
    createSwapchain();
    createRenderPass();
    createGraphicsPipeline(stride, offsetPos, offsetCol);
    createFramebuffers();
    createGraphicsCommandPool();
    createGraphicsCommandBuffer();
    recordCommands(vertexBuffer, verticesSize);
    createSynchronisation();
}

void VkGraphics::clean()
{
    renderer->mainDevices.device.waitIdle();
    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        renderer->mainDevices.device.destroySemaphore(renderFinished[i]);
        renderer->mainDevices.device.destroySemaphore(imageAvailable[i]);
        renderer->mainDevices.device.destroyFence(drawFences[i]);
    }
    renderer->mainDevices.device.destroyCommandPool(graphicsCommandPool);
    for (auto framebuffer : swapchainFramebuffers)
    {
        renderer->mainDevices.device.destroyFramebuffer(framebuffer);
    }
    renderer->mainDevices.device.destroySwapchainKHR(swapchain);
    for (auto image : swapchainImages)
    {
        renderer->mainDevices.device.destroyImageView(image.imageView);
    }
    renderer->mainDevices.device.destroyPipelineLayout(pipelineLayout);
    renderer->mainDevices.device.destroyRenderPass(renderPass);
    renderer->mainDevices.device.destroyPipeline(graphicsPipeline);
}

void VkGraphics::createSwapchain()
{
    SwapchainDetails swapchainDetails = renderer->getSwapchainDetails();
    vk::SurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapchainDetails.formats);
    vk::PresentModeKHR presentationMode = chooseBestPresentationMode(swapchainDetails.presentationModes);
    vk::Extent2D extent = chooseSwapExtent(swapchainDetails.surfaceCapabilities);

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = vk::StructureType::eSwapchainCreateInfoKHR;
    swapchainCreateInfo.surface = renderer->surface;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.presentMode = presentationMode;
    swapchainCreateInfo.imageExtent = extent;

    uint32_t imageCount = swapchainDetails.surfaceCapabilities.minImageCount + 1;
    if (swapchainDetails.surfaceCapabilities.maxImageCount > 0
        && swapchainDetails.surfaceCapabilities.maxImageCount < imageCount)
    {
        imageCount = swapchainDetails.surfaceCapabilities.maxImageCount;
    }
    swapchainCreateInfo.minImageCount = imageCount;

    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.preTransform = swapchainDetails.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCreateInfo.clipped = VK_TRUE;


    if (renderer->queueFamilyIndices.graphicsFamily != renderer->queueFamilyIndices.presentationFamily)
    {
        uint32_t queueFamilyIndicesArr[]{ renderer->queueFamilyIndices.graphicsFamily, renderer->queueFamilyIndices.presentationFamily };
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndicesArr;
    }
    else
    {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    swapchain = renderer->mainDevices.device.createSwapchainKHR(swapchainCreateInfo);

    swapchainImageFormat = surfaceFormat.format;
    swapchainExtent = extent;

    vector<vk::Image> images = renderer->mainDevices.device.getSwapchainImagesKHR(swapchain);
    for (vk::Image image : images)
    {
        SwapchainImage swapchainImage{};
        swapchainImage.image = image;
        swapchainImage.imageView = createImageView(image, swapchainImageFormat, vk::ImageAspectFlagBits::eColor);
        swapchainImages.push_back(swapchainImage);
    }
}

vk::SurfaceFormatKHR VkGraphics::chooseBestSurfaceFormat(const vector<vk::SurfaceFormatKHR>& formats)
{
    if (formats.size() == 1 && formats[0].format == vk::Format::eUndefined)
    {
        return { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }
    for (auto& format : formats)
    {
        if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
        {
            return format;
        }
    }

    return formats[0];
}

vk::PresentModeKHR VkGraphics::chooseBestPresentationMode(const vector<vk::PresentModeKHR>& presentationModes)
{
    for (const auto& presentationMode : presentationModes)
    {
        if (presentationMode == vk::PresentModeKHR::eMailbox)
        {
            return presentationMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VkGraphics::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& surfaceCapabilities)
{
    if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return surfaceCapabilities.currentExtent;
    }
    else
    {
        int width, height;
        glfwGetFramebufferSize(renderer->window, &width, &height);
        VkExtent2D newExtent{};
        newExtent.width = static_cast<uint32_t>(width);
        newExtent.height = static_cast<uint32_t>(height);

        newExtent.width = std::max(surfaceCapabilities.minImageExtent.width,
            std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width));
        newExtent.height = std::max(surfaceCapabilities.minImageExtent.height,
            std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));
        return newExtent;
    }
}

vk::ImageView VkGraphics::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags)
{
    vk::ImageViewCreateInfo imageViewCreateInfo{};
    imageViewCreateInfo.sType = vk::StructureType::eImageViewCreateInfo;
    imageViewCreateInfo.image = image;
    imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
    imageViewCreateInfo.format = format;
    imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    imageViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreateInfo.subresourceRange.levelCount = 1;
    imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreateInfo.subresourceRange.layerCount = 1;

    vk::ImageView imageView;
    imageView = renderer->mainDevices.device.createImageView(imageViewCreateInfo);

    return imageView;
}

void VkGraphics::createGraphicsPipeline(uint32_t stride, size_t offsetPos, size_t offsetCol)
{
    auto vertexShaderCode = readShaderFile("shaders/vert.spv");
    auto fragmentShaderCode = readShaderFile("shaders/frag.spv");
    vk::ShaderModule vertexShaderModule = renderer->createShader(vertexShaderCode);
    vk::ShaderModule fragmentShaderModule = renderer->createShader(fragmentShaderCode);

    vk::PipelineShaderStageCreateInfo vertexShaderCreateInfo{};
    vertexShaderCreateInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    vertexShaderCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertexShaderCreateInfo.module = vertexShaderModule;
    vertexShaderCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragmentShaderCreateInfo{};
    fragmentShaderCreateInfo.sType = vk::StructureType::ePipelineShaderStageCreateInfo;
    fragmentShaderCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragmentShaderCreateInfo.module = fragmentShaderModule;
    fragmentShaderCreateInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[]{
    vertexShaderCreateInfo, fragmentShaderCreateInfo
    };

    // -- VERTEX INPUT STAGE --
    vk::VertexInputBindingDescription vertexBinding{};
    vertexBinding.binding = 0;
    vertexBinding.stride = stride;
    vertexBinding.inputRate = vk::VertexInputRate::eVertex;

    std::array<vk::VertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[0].offset = offsetPos;

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
    attributeDescriptions[1].offset = offsetCol;

    vk::PipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = vk::StructureType::ePipelineVertexInputStateCreateInfo;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &vertexBinding;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // -- INPUT ASSEMBLY --
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
    inputAssemblyCreateInfo.sType = vk::StructureType::ePipelineInputAssemblyStateCreateInfo;
    inputAssemblyCreateInfo.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

    // -- VIEWPORT AND SCISSOR --
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor{};
    scissor.offset.setX(0);
    scissor.offset.setY(0);
    scissor.extent = swapchainExtent;
    vk::PipelineViewportStateCreateInfo viewportStateCreateInfo{};
    viewportStateCreateInfo.sType = vk::StructureType::ePipelineViewportStateCreateInfo;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;

    // -- RASTERIZER --
    vk::PipelineRasterizationStateCreateInfo rasterizerCreateInfo{};
    rasterizerCreateInfo.sType = vk::StructureType::ePipelineRasterizationStateCreateInfo;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = vk::PolygonMode::eFill;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = vk::CullModeFlagBits::eBack;
    rasterizerCreateInfo.frontFace = vk::FrontFace::eClockwise;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

    // -- MULTISAMPLING --
    vk::PipelineMultisampleStateCreateInfo multisamplingCreateInfo{};
    multisamplingCreateInfo.sType = vk::StructureType::ePipelineMultisampleStateCreateInfo;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples = vk::SampleCountFlagBits::e1;

    // -- BLENDING --
    vk::PipelineColorBlendStateCreateInfo colorBlendingCreateInfo{};
    colorBlendingCreateInfo.sType = vk::StructureType::ePipelineColorBlendStateCreateInfo;
    colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = VK_TRUE;
    // Blending equation:
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;
    colorBlendingCreateInfo.attachmentCount = 1;
    colorBlendingCreateInfo.pAttachments = &colorBlendAttachment;

    // -- PIPELINE LAYOUT --
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
    pipelineLayoutCreateInfo.sType = vk::StructureType::ePipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.setLayoutCount = 0;
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    pipelineLayout = renderer->mainDevices.device.createPipelineLayout(pipelineLayoutCreateInfo);

    // -- GRAPHICS PIPELINE CREATION --
    vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo{};
    graphicsPipelineCreateInfo.sType = vk::StructureType::eGraphicsPipelineCreateInfo;
    graphicsPipelineCreateInfo.stageCount = 2;
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
    graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    graphicsPipelineCreateInfo.pDynamicState = nullptr;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
    graphicsPipelineCreateInfo.pDepthStencilState = nullptr;
    graphicsPipelineCreateInfo.layout = pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineCreateInfo.basePipelineIndex = -1;


    vk::Result result = renderer->mainDevices.device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &graphicsPipeline);
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error("Cound not create a graphics pipeline");
    }

    renderer->mainDevices.device.destroyShaderModule(fragmentShaderModule);
    renderer->mainDevices.device.destroyShaderModule(vertexShaderModule);
}

void VkGraphics::createRenderPass()
{
    vk::RenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = vk::StructureType::eRenderPassCreateInfo;

    vk::AttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;

    vk::AttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpass{};
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;

    std::array<vk::SubpassDependency, 2> subpassDependencies;

    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].srcStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    subpassDependencies[0].srcAccessMask = vk::AccessFlagBits::eMemoryRead;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDependencies[0].dstAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
    subpassDependencies[0].dependencyFlags = vk::DependencyFlags::Flags();
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    subpassDependencies[1].srcAccessMask = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = vk::PipelineStageFlagBits::eBottomOfPipe;
    subpassDependencies[1].dstAccessMask = vk::AccessFlagBits::eMemoryRead;
    subpassDependencies[1].dependencyFlags = vk::DependencyFlags::Flags();
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    renderPass = renderer->mainDevices.device.createRenderPass(renderPassCreateInfo);

}

void VkGraphics::createFramebuffers()
{
    swapchainFramebuffers.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainFramebuffers.size(); ++i)
    {
        std::array<vk::ImageView, 1> attachments{ swapchainImages[i].imageView };
        vk::FramebufferCreateInfo framebufferCreateInfo{};
        framebufferCreateInfo.sType = vk::StructureType::eFramebufferCreateInfo;
        framebufferCreateInfo.renderPass = renderPass;
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();
        framebufferCreateInfo.width = swapchainExtent.width;
        framebufferCreateInfo.height = swapchainExtent.height;
        framebufferCreateInfo.layers = 1;

        swapchainFramebuffers[i] = renderer->mainDevices.device.createFramebuffer(framebufferCreateInfo);
    }
}

void VkGraphics::createGraphicsCommandPool()
{
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.sType = vk::StructureType::eCommandPoolCreateInfo;
    poolInfo.queueFamilyIndex = renderer->queueFamilyIndices.graphicsFamily;

    graphicsCommandPool = renderer->mainDevices.device.createCommandPool(poolInfo);
}

void VkGraphics::createGraphicsCommandBuffer()
{
    commandBuffers.resize(swapchainFramebuffers.size());
    vk::CommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = vk::StructureType::eCommandBufferAllocateInfo;
    commandBufferAllocInfo.commandPool = graphicsCommandPool;
    commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
    commandBufferAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    commandBuffers = renderer->mainDevices.device.allocateCommandBuffers(commandBufferAllocInfo);
}

void VkGraphics::recordCommands(vk::Buffer vertexBuffer, uint32_t verticesSize)
{
    vk::CommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = vk::StructureType::eCommandBufferBeginInfo;

    vk::RenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = vk::StructureType::eRenderPassBeginInfo;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.renderArea.offset.x = 0;
    renderPassBeginInfo.renderArea.offset.y = 0;
    renderPassBeginInfo.renderArea.extent = swapchainExtent;
    const vk::ClearValue clearValues{
    std::array<float,4>{0.f, 0.f, .4f, 1.0f}
    };

    renderPassBeginInfo.pClearValues = &clearValues;
    renderPassBeginInfo.clearValueCount = 1;


    for (size_t i = 0; i < commandBuffers.size(); ++i)
    {
        renderPassBeginInfo.framebuffer = swapchainFramebuffers[i];
        commandBuffers[i].begin(commandBufferBeginInfo);
        commandBuffers[i].beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

        vk::DeviceSize offsets[] = { 0 };
        vk::Buffer vertexBuffers[] = { vertexBuffer };

        commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffers[i].draw(static_cast<uint32_t>(verticesSize), 1, 0, 0);
        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }
}

void VkGraphics::draw()
{
    renderer->mainDevices.device.waitForFences(drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint32_t>::max());
    renderer->mainDevices.device.resetFences(drawFences[currentFrame]);
    uint32_t imageToBeDrawnIndex;

    vk::ResultValue result = renderer->mainDevices.device.acquireNextImageKHR(swapchain, std::numeric_limits<uint32_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE);
    imageToBeDrawnIndex = result.value;

    vk::SubmitInfo submitInfo{};
    submitInfo.sType = vk::StructureType::eSubmitInfo;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageAvailable[currentFrame];

    vk::PipelineStageFlags waitStages[]{ vk::PipelineStageFlagBits::eColorAttachmentOutput };
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[imageToBeDrawnIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinished[currentFrame];

    renderer->graphicsQueue.submit(submitInfo, drawFences[currentFrame]);

    vk::PresentInfoKHR presentInfo{};
    presentInfo.sType = vk::StructureType::ePresentInfoKHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageToBeDrawnIndex;

    renderer->presentationQueue.presentKHR(presentInfo);

    currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VkGraphics::createSynchronisation()
{
    imageAvailable.resize(MAX_FRAME_DRAWS);
    renderFinished.resize(MAX_FRAME_DRAWS);
    drawFences.resize(MAX_FRAME_DRAWS);

    vk::SemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = vk::StructureType::eSemaphoreCreateInfo;

    vk::FenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = vk::StructureType::eFenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
        imageAvailable[i] = renderer->mainDevices.device.createSemaphore(semaphoreCreateInfo);
        renderFinished[i] = renderer->mainDevices.device.createSemaphore(semaphoreCreateInfo);
        drawFences[i] = renderer->mainDevices.device.createFence(fenceCreateInfo);
    }
}
