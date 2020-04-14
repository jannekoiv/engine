
#include "../Include/Pipeline.h"
#include "../Include/Base.h"
#include "../Include/Device.h"
#include "../Include/Material.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vulkan/vulkan.hpp>

Pipeline::Pipeline(Pipeline&& rhs)
    : mDevice{rhs.mDevice},
      mFramebufferSet{std::move(rhs.mFramebufferSet)},
      mPipelineLayout{rhs.mPipelineLayout},
      mPipeline{rhs.mPipeline}
{
    rhs.mPipelineLayout = nullptr;
    rhs.mPipeline = nullptr;
}

vk::PipelineLayout createPipelineLayout(
    Device& device,
    Material& material,
    vk::DescriptorSetLayout descriptorSetLayout,
    const MaterialUsage usage,
    const nlohmann::json& json)
{
    std::vector<vk::DescriptorSetLayout> layouts{};
    if (descriptorSetLayout) {
        layouts.push_back(descriptorSetLayout);
    }

    if (usage != MaterialUsage::ShadowMap) {
        layouts.push_back(material.descriptorSet().layout());
    }

    vk::PushConstantRange pushConstantRange{};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 16;
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.setLayoutCount = layouts.size();
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    vk::PipelineLayout pipelineLayout = static_cast<vk::Device>(device).createPipelineLayout(pipelineLayoutInfo);
    return pipelineLayout;
}

vk::Pipeline createPipeline(
    Device& device,
    Material& material,
    FramebufferSet& framebufferSet,
    vk::VertexInputBindingDescription bindingDescription,
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions,
    vk::Extent2D swapChainExtent,
    vk::PipelineLayout pipelineLayout,
    const MaterialUsage usage,
    const nlohmann::json& json)
{
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = material.vertexShader();
    vertShaderStageInfo.pName = "main";

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        vertShaderStageInfo,
    };

    if (usage != MaterialUsage::ShadowMap) {
        vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
        fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragShaderStageInfo.module = material.fragmentShader();
        fragShaderStageInfo.pName = "main";
        shaderStages.push_back(fragShaderStageInfo);
    }

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::Viewport viewport;
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = swapChainExtent;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = false;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.0;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = false;
    multisampling.alphaToOneEnable = false;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eB;
    colorBlendAttachment.blendEnable = false;

    std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments = {colorBlendAttachment};

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = false;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlending.pAttachments = colorBlendAttachments.data();

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = true;

    if (usage == MaterialUsage::Skybox) {
        depthStencil.depthCompareOp = vk::CompareOp::eLessOrEqual;
    } else if (usage == MaterialUsage::Model) {
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
    } else if (usage == MaterialUsage::Quad) {
        depthStencil.depthTestEnable = false;
        depthStencil.depthWriteEnable = false;
    }

    depthStencil.depthBoundsTestEnable = false;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;
    depthStencil.stencilTestEnable = false;
    depthStencil.front = vk::StencilOpState{};
    depthStencil.back = vk::StencilOpState{};

    if (usage == MaterialUsage::ShadowMap) {
        depthStencil.depthCompareOp = vk::CompareOp::eLess;
        depthStencil.depthTestEnable = true;
        depthStencil.depthWriteEnable = true;
    }

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = shaderStages.size();
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = framebufferSet.renderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    vk::Pipeline pipeline = static_cast<vk::Device>(device).createGraphicsPipeline(nullptr, pipelineInfo, nullptr);
    return pipeline;
}

Pipeline::Pipeline(
    Device& device,
    Material& material,
    SwapChain& swapChain,
    Texture* depthTexture,
    vk::DescriptorSetLayout descriptorSetLayout,
    vk::VertexInputBindingDescription bindingDescription,
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions,
    const MaterialUsage usage,
    const nlohmann::json& json)
    : mDevice{device},
      mFramebufferSet{mDevice, swapChain, depthTexture, usage},
      mPipelineLayout{createPipelineLayout(mDevice, material, descriptorSetLayout, usage, json)},
      mPipeline{createPipeline(
          mDevice,
          material,
          mFramebufferSet,
          bindingDescription,
          attributeDescriptions,
          swapChain.extent(),
          mPipelineLayout,
          usage,
          json)}
{
    std::cout << "Pipeline constructed.\n";
}

Pipeline::~Pipeline()
{
    static_cast<vk::Device>(mDevice).destroyPipeline(mPipeline);
    static_cast<vk::Device>(mDevice).destroyPipelineLayout(mPipelineLayout);
}
