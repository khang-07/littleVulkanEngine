#pragma once

#include "lve_device.hpp"

#include <string>
#include <vector>

namespace lve {
// outside of class bc used with many pipelines
struct PipelineConfigInfo {
    // to be config-ed in defaultPipelineConfigInfo()
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineViewportStateCreateInfo viewportInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationInfo;
    VkPipelineMultisampleStateCreateInfo multisampleInfo;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
    VkPipelineLayout pipelineLayout = nullptr;
    VkRenderPass renderPass = nullptr;
    uint32_t subpass = 0;
};

class LvePipeline {
    public:
    LvePipeline(
        LveDevice &device, 
        const std::string& vertFilepath,  
        const std::string& fragFilepath, 
        const PipelineConfigInfo& configInfo);

    ~LvePipeline();
    
    LvePipeline(const LvePipeline &) = delete;
    void operator=(const LvePipeline &) = delete;

    void bind(VkCommandBuffer commandBuffer);

    static PipelineConfigInfo defaultPipelineConfigInfo(
        uint32_t width, 
        uint32_t height);

    private:
    static std::vector<char> readFile(const std::string& filepath);
    void createGraphicsPipeline(
        const std::string& vertFilepath, 
        const std::string& fragFilepath,
        const PipelineConfigInfo& configInfo);

    void createShaderModule(
        const std::vector<char>& code, 
        VkShaderModule* shaderModule); // pointer2pointer

    LveDevice& lveDevice;
    // below are pointers
    VkPipeline graphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
};
}