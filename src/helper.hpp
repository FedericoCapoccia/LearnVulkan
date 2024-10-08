#pragma once

namespace Minecraft::VkEngine::VkInit {

inline vk::ImageCreateInfo image_create_info(const vk::Format format, const vk::ImageUsageFlags usage_flags, const vk::Extent3D extent)
{
    return vk::ImageCreateInfo {
        {},
        vk::ImageType::e2D,
        format,
        extent,
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        usage_flags
    };
}

inline vk::ImageViewCreateInfo imageview_create_info(const vk::Format format, const vk::Image image, const vk::ImageAspectFlags aspect_flags)
{
    return vk::ImageViewCreateInfo {
        {},
        image,
        vk::ImageViewType::e2D,
        format,
        {},
        vk::ImageSubresourceRange {
            aspect_flags,
            0, 1,
            0, 1 }
    };
}

inline vk::RenderingAttachmentInfo attachment_info(const vk::ImageView view, const vk::ClearValue* clear , const vk::ImageLayout layout)
{
    vk::RenderingAttachmentInfo colorAttachment {};
    colorAttachment.imageView = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    if (clear) {
        colorAttachment.clearValue = *clear;
    }

    return colorAttachment;
}

inline vk::RenderingInfo rendering_info(const vk::Extent2D render_extent, const vk::RenderingAttachmentInfo* color_attachment, const vk::RenderingAttachmentInfo* depth_attachment)
{
    vk::RenderingInfo info {};

    info.renderArea = vk::Rect2D { vk::Offset2D { 0, 0 }, render_extent };
    info.layerCount = 1;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = color_attachment;
    info.pDepthAttachment = depth_attachment;
    info.pStencilAttachment = nullptr;

    return info;
}

inline vk::PipelineLayoutCreateInfo pipeline_layout_create_info()
{
    vk::PipelineLayoutCreateInfo info {};
    info.setLayoutCount = 0;
    info.pSetLayouts = nullptr;
    info.pushConstantRangeCount = 0;
    info.pPushConstantRanges = nullptr;
    return info;
}

inline vk::PipelineShaderStageCreateInfo pipeline_shader_stage_create_info(const vk::ShaderStageFlagBits stage, const vk::ShaderModule shaderModule, const char * entry)
{
    vk::PipelineShaderStageCreateInfo info {};
    info.stage = stage;
    info.module = shaderModule;
    info.pName = entry;
    return info;
}

}

namespace Minecraft::VkEngine::VkUtil {

inline std::expected<vk::ShaderModule, std::string> load_shader_module(const char* filepath, const vk::Device device)
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        return std::unexpected(fmt::format("Unable to open shader file: {}", filepath));
    }

    const std::streamsize file_size = file.tellg();
    std::vector<char> buffer;
    buffer.resize(file_size);
    file.seekg(0); // return at beginning of file
    file.read(buffer.data(), file_size);
    file.close();

    const vk::ShaderModuleCreateInfo create_info { {},
        buffer.size(),
        reinterpret_cast<const uint32_t*>(buffer.data()) };

    const auto [result, shader_module] = device.createShaderModule(create_info);

    if (result != vk::Result::eSuccess) {
        return std::unexpected(vk::to_string(result));
    }

    return shader_module;
}

inline vk::ImageSubresourceRange image_subresource_range(vk::ImageAspectFlags aspect_mask)
{
    return {
        aspect_mask,
        0,
        vk::RemainingMipLevels,
        0,
        vk::RemainingArrayLayers
    };
}

inline void transition_image(const vk::CommandBuffer& cmd, const vk::Image& image, const vk::ImageLayout& src_layout, const vk::ImageLayout& dst_layout)
{
    // ReSharper disable once CppDFAConstantConditions
    const vk::ImageAspectFlags aspect_mask = dst_layout == vk::ImageLayout::eDepthAttachmentOptimal
        ? vk::ImageAspectFlagBits::eDepth
        : vk::ImageAspectFlagBits::eColor;

    vk::ImageMemoryBarrier2 image_barrier {
        vk::PipelineStageFlagBits2::eAllCommands, // TODO inefficient https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
        vk::AccessFlagBits2::eMemoryWrite,
        vk::PipelineStageFlagBits2::eAllCommands,
        vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
        src_layout, dst_layout,
        {}, {},
        image, image_subresource_range(aspect_mask)
    };

    const vk::DependencyInfo dependency_info {
        {},
        {}, {},
        {}, {},
        1, &image_barrier
    };

    cmd.pipelineBarrier2(dependency_info);
}

inline void copy_image_to_image(const vk::CommandBuffer cmd, const vk::Image source, const vk::Image destination, const vk::Extent2D src_size, const vk::Extent2D dst_size)
{
    constexpr vk::ImageSubresourceLayers src_subresource {
        vk::ImageAspectFlagBits::eColor,
        0, 0, 1
    };

    constexpr vk::ImageSubresourceLayers dst_subresource {
        vk::ImageAspectFlagBits::eColor,
        0, 0, 1
    };

    const std::array src_offset = {
        vk::Offset3D {},
        vk::Offset3D { static_cast<int32_t>(src_size.width), static_cast<int32_t>(src_size.height), 1 }
    };

    const std::array dst_offset = {
        vk::Offset3D {},
        vk::Offset3D { static_cast<int32_t>(src_size.width), static_cast<int32_t>(src_size.height), 1 }
    };

    const vk::ImageBlit2 blit_region {
        src_subresource, src_offset,
        dst_subresource, dst_offset
    };

    const vk::BlitImageInfo2 blit_info {
        source, vk::ImageLayout::eTransferSrcOptimal,
        destination, vk::ImageLayout::eTransferDstOptimal,
        1, &blit_region,
        vk::Filter::eNearest
    };

    cmd.blitImage2(blit_info);
}

}
