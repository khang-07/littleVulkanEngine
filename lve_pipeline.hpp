#pragma once

#include <string>
#include <vector>
#include "lve_device.hpp"

namespace lve {
// outside of class bc used with many pipelines
struct PipelineConfigInfo {};

class LvePipeline {
    public:
    LvePipeline(
        LveDevice &device, 
        const std::string& vertFilepath,  
        const std::string& fragFilepath, 
        const PipelineConfigInfo& configInfo);
    ~LvePipeline() {};
    
    LvePipeline(const LvePipeline &) = delete;
    LvePipeline operator=(const LvePipeline &) = delete;

    static PipelineConfigInfo defaultPipelineConfigInfo(u_int32_t width,u_int32_t height);

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