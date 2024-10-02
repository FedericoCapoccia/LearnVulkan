#include "images.hpp"
#include "vk_init.hpp"

namespace Minecraft::VkUtil {

void transition_image(const vk::CommandBuffer cmd, const vk::Image image, const vk::ImageLayout current_layout, const vk::ImageLayout new_layout)
{
    const vk::ImageAspectFlags aspectMask = (new_layout == vk::ImageLayout::eDepthAttachmentOptimal) ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor;

    const auto image_barrier = vk::ImageMemoryBarrier2(
        vk::PipelineStageFlagBits2::eAllCommands,
        vk::AccessFlagBits2::eMemoryWrite,
        vk::PipelineStageFlagBits2::eAllCommands,
        vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
        current_layout, new_layout,
        {}, {},
        image, VkInit::image_subresource_range(aspectMask));

    auto dep_info = vk::DependencyInfo();
    dep_info.imageMemoryBarrierCount = 1;
    dep_info.pImageMemoryBarriers = &image_barrier;

    cmd.pipelineBarrier2(&dep_info);
}

}
