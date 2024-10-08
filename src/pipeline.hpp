#pragma once

namespace Minecraft::VkEngine {

class PipelineBuilder {

public:
  PipelineBuilder(){ clear(); }

  std::expected<vk::Pipeline, vk::Result> build_pipeline(vk::Device device, vk::PipelineLayout layout);
  PipelineBuilder& set_shaders(vk::ShaderModule vertex_shader, vk::ShaderModule fragment_shader);
  PipelineBuilder& set_input_topology(vk::PrimitiveTopology topology);
  PipelineBuilder& set_polygon_mode(vk::PolygonMode mode);
  PipelineBuilder& set_cull_mode(vk::CullModeFlags cull_mode, vk::FrontFace front_face);
  PipelineBuilder& set_multisampling_none();
  PipelineBuilder& disable_blending();
  PipelineBuilder& set_color_attachment_format(vk::Format format);
  PipelineBuilder& set_depth_format(vk::Format format);
  PipelineBuilder& disable_depth_test();

private:
  std::vector<vk::PipelineShaderStageCreateInfo> ShaderStages{};

  vk::PipelineInputAssemblyStateCreateInfo InputAssembly;
  vk::PipelineRasterizationStateCreateInfo Rasterizer;
  vk::PipelineColorBlendAttachmentState ColorBlendAttachment;
  vk::PipelineMultisampleStateCreateInfo Multisampling;
  vk::PipelineDepthStencilStateCreateInfo DepthStencil;
  vk::PipelineRenderingCreateInfo RenderInfo;
  vk::Format ColorAttachmentFormat {};

  void clear();
};

}