#include "first_app.hpp"

#include <stdexcept>
#include <array>

namespace lve {

FirstApp::FirstApp() {
    createPipelineLayout();
    createPipeline();
    createCommandBuffers();
}

FirstApp::~FirstApp() {
    vkDestroyPipelineLayout(lveDevice.device(), pipelineLayout, nullptr);
}

void FirstApp::run() {
    while (!lveWindow.shouldClose()) {
        glfwPollEvents();
        drawFrame();
    }

    vkDeviceWaitIdle(lveDevice.device());
}

void FirstApp::createPipelineLayout() {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
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
  PipelineConfigInfo pipelineConfig;
  LvePipeline::defaultPipelineConfigInfo(
      pipelineConfig,
      lveSwapChain.width(),
      lveSwapChain.height());
    pipelineConfig.renderPass = lveSwapChain.getRenderPass();
    pipelineConfig.pipelineLayout = pipelineLayout;
    lvePipeline = std::make_unique<LvePipeline>(
        lveDevice,
        "./shaders/simple_shader.vert.spv",
        "./shaders/simple_shader.frag.spv",
        pipelineConfig);
}

void FirstApp::createCommandBuffers() { // meanwhile, void LveDevice::createCommandPool() {}
    commandBuffers.resize(lveSwapChain.imageCount());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = lveDevice.getCommandPool(); // use cmd pool mem for cmd buff
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(lveDevice.device(), &allocInfo, commandBuffers.data())
    != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers");
    }

    for (int i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo)
        != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = lveSwapChain.getRenderPass();
        renderPassInfo.framebuffer = lveSwapChain.getFrameBuffer(i);

        renderPassInfo.renderArea.offset = {0, 0}; // origin + borders(ish)
        renderPassInfo.renderArea.extent = lveSwapChain.getSwapChainExtent();

        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = {0.1f, 0.1f, 0.1f, 1.0}; // R G B A
        clearValues[1].depthStencil = {1.0f, 0};
        // [0] color : [1] depth -> find code
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

        // inline : only primary
        vkCmdBeginRenderPass(commandBuffers[1], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        lvePipeline->bind(commandBuffers[i]);
        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0); // 3 vertices : 1 instance : 0 offsets : 0 First Instance?
        
        vkCmdEndRenderPass(commandBuffers[i]);
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer");
        }
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