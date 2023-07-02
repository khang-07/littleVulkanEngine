#include "lve_pipeline.hpp"
#include "lve_model.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cassert>

namespace lve {

LvePipeline::LvePipeline(
    LveDevice &device, 
    const std::string& vertFilepath,  
    const std::string& fragFilepath, 
    const PipelineConfigInfo& configInfo)
    : lveDevice{device} {
    createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
    }

LvePipeline::~LvePipeline() {
    vkDestroyShaderModule(lveDevice.device(), vertShaderModule, nullptr);
    vkDestroyShaderModule(lveDevice.device(), fragShaderModule, nullptr);
    vkDestroyPipeline(lveDevice.device(), graphicsPipeline, nullptr);
}

std::vector<char> LvePipeline::readFile(const std::string& filepath) {
    std::ifstream file{filepath, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filepath);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

void LvePipeline::createGraphicsPipeline(
    const std::string& vertFilepath, 
    const std::string& fragFilepath,
    const PipelineConfigInfo& configInfo) {
    auto vertCode = readFile(vertFilepath);
    auto fragCode = readFile(fragFilepath);

    assert(
    configInfo.pipelineLayout != VK_NULL_HANDLE && 
    "Cannot create graphics pipeline::  no pipelineLayout provided in configInfo");

    assert(
    configInfo.renderPass != VK_NULL_HANDLE && 
    "Cannot create graphics pipeline::  no renderPass provided in configInfo");

    createShaderModule(vertCode, &vertShaderModule);
    createShaderModule(fragCode, &fragShaderModule);

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[0].flags = 0;
    shaderStages[0].pNext = nullptr;
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragShaderModule;
    shaderStages[1].pName = "main";
    shaderStages[1].flags = 0;
    shaderStages[1].pNext = nullptr;
    shaderStages[1].pSpecializationInfo = nullptr;
    
    auto bindingDescriptions = LveModel::Vertex::getBindingDescriptions();
    auto attributeDescriptions = LveModel::Vertex::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();

    // transfer configInfo.* to pipelineInfo.*
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages; // pStages = programmable stages
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
    pipelineInfo.pViewportState = &configInfo.viewportInfo;
    pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
    pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
    pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
    pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
    pipelineInfo.pDynamicState = nullptr; // reconfigure pipeline function w/o recreating pipeline

    // not ptr : not configured yet
    pipelineInfo.layout = configInfo.pipelineLayout;
    pipelineInfo.renderPass = configInfo.renderPass;
    pipelineInfo.subpass = configInfo.subpass;

    // used 4 optimize : create new pipeline from another
    pipelineInfo.basePipelineIndex = -1;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    // 1=1 pipeline : nullptr=no alloc callback : 
    if (vkCreateGraphicsPipelines(
        lveDevice.device(), 
        VK_NULL_HANDLE, 
        1, 
        &pipelineInfo, 
        nullptr,
        &graphicsPipeline) !=  VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline");
    }

}

void LvePipeline::createShaderModule(
    const std::vector<char>& code, 
    VkShaderModule* shaderModule) {
    // instead of calling function(), use struct{}
    VkShaderModuleCreateInfo createInfo = {};
        
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const u_int32_t*>(code.data());

    if (vkCreateShaderModule(lveDevice.device(), &createInfo, nullptr, shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module");
    }
}

void LvePipeline::bind(VkCommandBuffer commandBuffer) {
    // other pipeline points : compute, ray trace
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
}

PipelineConfigInfo LvePipeline::defaultPipelineConfigInfo(uint32_t width, uint32_t height) {
    PipelineConfigInfo configInfo = {};

    configInfo.inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // render w triangles
    configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; // break up strip d'triangle

    // configInfo.inputAssemblyInfo.pNext = NULL;

    // desc trans bw pipeline-out, target-in
    configInfo.viewport.x = 0.0f;
    configInfo.viewport.y = 0.0f;

    // desc width, height (if != window dimensions) rendering = squished
    configInfo.viewport.width = static_cast<float>(width);
    configInfo.viewport.height = static_cast<float>(height);
    configInfo.viewport.minDepth = 0.0f;
    configInfo.viewport.maxDepth = 1.0f;
    
    // cuts off instead of squish
    configInfo.scissor.offset = {0, 0};
    configInfo.scissor.extent = {width, height};

    configInfo.viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    configInfo.viewportInfo.viewportCount = 1;
    configInfo.viewportInfo.pViewports = &configInfo.viewport;
    configInfo.viewportInfo.scissorCount = 1;
    configInfo.viewportInfo.pScissors = &configInfo.scissor;

    configInfo.rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    configInfo.rasterizationInfo.depthClampEnable = VK_FALSE; // below 0 behind camera : above 1 too far
    configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // DONT discard all prims b4 raster
    configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    configInfo.rasterizationInfo.lineWidth = 1.0f;
    configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE; // apparent order : determine which side is front/back
    configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE; // enhance depth
    configInfo.rasterizationInfo.depthBiasConstantFactor = 0.0f; // optional
    configInfo.rasterizationInfo.depthBiasClamp = 0.0f; // optional
    configInfo.rasterizationInfo.depthBiasSlopeFactor = 0.0f; // optional

    // handle edges of triangles
    configInfo.multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
    configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    configInfo.multisampleInfo.minSampleShading = 1.0f; // optional
    configInfo.multisampleInfo.pSampleMask = nullptr; // optional
    configInfo.multisampleInfo.alphaToCoverageEnable = VK_FALSE; // optional
    configInfo.multisampleInfo.alphaToOneEnable = VK_FALSE; // optional

    configInfo.colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;
    configInfo.colorBlendAttachment.blendEnable = VK_FALSE;
    configInfo.colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // optional
    configInfo.colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    configInfo.colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // optional
    configInfo.colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
    configInfo.colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
    configInfo.colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // optional
    
    configInfo.colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
    configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_COPY; // optional
    configInfo.colorBlendInfo.attachmentCount = 1;
    configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
    configInfo.colorBlendInfo.blendConstants[0] = 0.0f; // optional
    configInfo.colorBlendInfo.blendConstants[1] = 0.0f; // optional
    configInfo.colorBlendInfo.blendConstants[2] = 0.0f; // optional
    configInfo.colorBlendInfo.blendConstants[3] = 0.0f; // optional

    // stores depth val for every pixel comme every pixel has color val
    // discards pixels that are being covered
    configInfo.depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
    configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.minDepthBounds = 0.0f; // optional
    configInfo.depthStencilInfo.maxDepthBounds = 1.0f; // optional
    configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;
    configInfo.depthStencilInfo.front = {}; // optional
    configInfo.depthStencilInfo.back = {}; // optional

    return configInfo;
}

}
