#ifndef VK_INIT_HPP
#define VK_INIT_HPP

namespace Minecraft::VkInit {

inline vk::ImageSubresourceRange image_subresource_range(vk::ImageAspectFlags aspect_mask)
{
    return {
        aspect_mask,
        0, vk::RemainingMipLevels,
        0, vk::RemainingArrayLayers
    };
}

inline vk::SemaphoreSubmitInfo semaphore_submit_info(vk::PipelineStageFlags2 stage_mask, vk::Semaphore semaphore)
{
    return {
        semaphore, 1, stage_mask, 0
    };
}

inline vk::CommandBufferSubmitInfo command_buffer_submit_info(vk::CommandBuffer cmd)
{
    return { cmd, 0 };
}

inline vk::SubmitInfo2 submit_info(vk::CommandBufferSubmitInfo* cmd, vk::SemaphoreSubmitInfo* signal_semaphore_info,
    vk::SemaphoreSubmitInfo* wait_semaphore_info)
{
    return {
        {},
        static_cast<uint32_t>(wait_semaphore_info == nullptr ? 0 : 1),
        wait_semaphore_info,
        1, cmd,
        static_cast<uint32_t>(signal_semaphore_info == nullptr ? 0 : 1),
        signal_semaphore_info
    };
}

}

#endif // VK_INIT_HPP
