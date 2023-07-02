#include "first_app.hpp"

#include <array>

namespace lve {

FirstApp::FirstApp() {
    loadModels();
    createPipelineLayout();
    createPipeline();
    createCommandBuffers();
}

FirstApp::~FirstApp() {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
    vkFreeCommandBuffers(
        lveDevice.device(),
        lveDevice.getCommandPool(),
        static_cast<uint32_t>(commandBuffers.size()),
        commandBuffers.data());
    commandBuffers.clear();
}

void FirstApp::loadModels() {
    std::vector<LveModel::Vertex> vertices{
        {{0.0f, -0.5f}},
        {{0.5f, 0.5f}},
        {{-0.5f, 0.5f}}
    };

    lveModel = std::make_unique<LveModel>(lveDevice, vertices);
}

void FirstApp::run() {
    while (!lveWindow.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // bc empty layout : no textures yet
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // set small data 2 shader
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    
    if (vkCreatePipelineLayout(lveDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout)
    != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout");
    }
}

void FirstApp::createPipeline() {
    auto pipelineConfig = LvePipeline::defaultPipelineConfigInfo(
        lveSwapChain.width(),
        lveSwapChain.height());
    pipelineConfig.renderPass = lveSwapChain.getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    simplePipeline = std::make_unique<LvePipeline>(
        lveDevice,
        "./shaders/simple_shader.vert.spv",
        "./shaders/simple_shader.frag.spv",
        pipelineConfig);
}

void FirstApp::createCommandBuffers() { // meanwhile, void LveDevice::createCommandPool() {}
    commandBuffers.resize(lveSwapChain.imageCount());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = lveDevice.getCommandPool(); // use cmd pool mem for cmd buff
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data())
    != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        
        VkRenderPassBeginInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = lveSwapChain.getRenderPass();
        renderPassInfo.framebuffer = lveSwapChain.getFrameBuffer(i);

        renderPassInfo.renderArea.offset = {0, 0}; // origin + borders(ish)
        renderPassInfo.renderArea.extent = lveSwapChain.getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues = {};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0}; // R G B A
        clearValues[1].depthStencil = {1.0f, 0};
        // [0] color : [1] depth -> find code
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // inline : only primary
        vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        simplePipeline->bind(commandBuffers[i]);
        lveModel->bind(commandBuffers[i]);
        lveModel->draw(commandBuffers[i]);

        vkCmdEndRenderPass(commandBuffers[i]);
        vkEndCommandBuffer(commandBuffers[i]);
    }
}

void FirstApp::drawFrame() {
    uint32_t imageIndex;
    auto result = lveSwapChain.acquireNextImage(&imageIndex);

    if (result != VK_SUCCESS || result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image");
    }
    
    // submit + handle CPU-GPU sync
    result = lveSwapChain.submitCommandBuffers(&commandBuffers[imageIndex], &imageIndex);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image");
    }
}
} 