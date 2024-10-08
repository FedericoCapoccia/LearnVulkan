#include "pipeline.hpp"

#include "helper.hpp"

namespace Minecraft::VkEngine {

void PipelineBuilder::clear()
{
    InputAssembly = vk::PipelineInputAssemblyStateCreateInfo {};
    Rasterizer = vk::PipelineRasterizationStateCreateInfo {};
    ColorBlendAttachment = vk::PipelineColorBlendAttachmentState {};
    Multisampling = vk::PipelineMultisampleStateCreateInfo {};
    DepthStencil = vk::PipelineDepthStencilStateCreateInfo {};
    RenderInfo = vk::PipelineRenderingCreateInfo {};
    ShaderStages.resize(2);
    ShaderStages.clear();
}

std::expected<vk::Pipeline, vk::Result> PipelineBuilder::build_pipeline(const vk::Device device, const vk::PipelineLayout layout)
{
    constexpr vk::PipelineViewportStateCreateInfo viewport_state { {},
        1, {},
        1, {} };

    const vk::PipelineColorBlendStateCreateInfo color_blending { {},
        vk::False,
        vk::LogicOp::eCopy,
        1, &ColorBlendAttachment };

    constexpr vk::PipelineVertexInputStateCreateInfo vertex_input_info {};

    constexpr vk::DynamicState state[] = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamic_info { {},
        2, &state[0] };

    vk::GraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.pNext = &RenderInfo;

    pipeline_info.stageCount = static_cast<uint32_t>(ShaderStages.size());
    pipeline_info.pStages = ShaderStages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &InputAssembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &Rasterizer;
    pipeline_info.pMultisampleState = &Multisampling;
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDepthStencilState = &DepthStencil;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.layout = layout;

    const auto [res, new_pipeline] = device.createGraphicsPipeline(VK_NULL_HANDLE, pipeline_info);

    if (res != vk::Result::eSuccess) {
        return std::unexpected(res);
    }

    return new_pipeline;
}

PipelineBuilder& PipelineBuilder::set_shaders(const vk::ShaderModule vertex_shader, const vk::ShaderModule fragment_shader)
{
    ShaderStages.clear();
    ShaderStages.emplace_back(VkInit::pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, vertex_shader, "main" ));
    ShaderStages.emplace_back(VkInit::pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, fragment_shader, "main" ));
    return *this;
}

PipelineBuilder& PipelineBuilder::set_input_topology(const vk::PrimitiveTopology topology)
{
    InputAssembly.topology = topology;
    InputAssembly.primitiveRestartEnable = vk::False;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_polygon_mode(const vk::PolygonMode mode)
{
    Rasterizer.polygonMode = mode;
    Rasterizer.lineWidth = 1.0f;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_cull_mode(const vk::CullModeFlags cull_mode, const vk::FrontFace front_face)
{
    Rasterizer.cullMode = cull_mode;
    Rasterizer.frontFace = front_face;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_multisampling_none()
{
    Multisampling.sampleShadingEnable = vk::False;
    Multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    Multisampling.minSampleShading = 1.0f;
    Multisampling.pSampleMask = nullptr;
    Multisampling.alphaToCoverageEnable = vk::False;
    Multisampling.alphaToOneEnable = vk::False;
    return *this;
}

PipelineBuilder& PipelineBuilder::disable_blending()
{
    vk::ColorComponentFlags mask {};
    mask |= vk::ColorComponentFlagBits::eR;
    mask |= vk::ColorComponentFlagBits::eG;
    mask |= vk::ColorComponentFlagBits::eB;
    mask |= vk::ColorComponentFlagBits::eA;

    ColorBlendAttachment.colorWriteMask = mask;
    ColorBlendAttachment.blendEnable = vk::False;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_color_attachment_format(const vk::Format format)
{
    ColorAttachmentFormat = format;
    RenderInfo.colorAttachmentCount = 1;
    RenderInfo.pColorAttachmentFormats = &ColorAttachmentFormat;
    return *this;
}

PipelineBuilder& PipelineBuilder::set_depth_format(const vk::Format format)
{
    RenderInfo.depthAttachmentFormat = format;
    return *this;
}

PipelineBuilder& PipelineBuilder::disable_depth_test()
{
    DepthStencil.depthTestEnable = vk::False;
    DepthStencil.depthWriteEnable = vk::False;
    DepthStencil.depthCompareOp = vk::CompareOp::eNever;
    DepthStencil.depthBoundsTestEnable = vk::False;
    DepthStencil.stencilTestEnable = vk::False;
    //DepthStencil.front
    //DepthStencil.back
    DepthStencil.minDepthBounds = 0.0f;
    DepthStencil.maxDepthBounds = 1.0f;
    return *this;
}

}
