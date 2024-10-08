#pragma once
#include "types.hpp"

namespace Minecraft::VkEngine {

class GpuManager {
public:
    GpuManager() { fmt::println("Gpu manager created"); }
    ResourcesBundle init(const GpuManagerSpec& spec);
    void destroy();
    void wait_idle() const;

    // Swapchain
    std::expected<vk::Image, vk::Result> get_next_swapchain_image(vk::Semaphore swapchain_semaphore, uint64_t timeout);
    [[nodiscard]] vk::Extent2D get_swapchain_extent() const { return m_SwapchainBundle.Extent; }

    // Queue
    //[[nodiscard]] uint32_t get_graphics_queue_index() const { return m_GraphicsQueue.FamilyIndex; }
    [[nodiscard]] vk::Result submit_to_queue(const vk::SubmitInfo2& submit_info2, vk::Fence render_fence) const;
    vk::Result present(uint32_t semaphores_count, vk::Semaphore* semaphores);

    // Sync Structures
    [[nodiscard]] std::expected<vk::CommandPool, vk::Result> create_command_pool(vk::CommandPoolCreateFlags flags);
    // TODO make it support multiple command buffers allocations if needed
    [[nodiscard]] std::expected<vk::CommandBuffer, vk::Result> allocate_command_buffer(vk::CommandPool pool, vk::CommandBufferLevel level) const;
    std::expected<vk::Semaphore, vk::Result> create_semaphore(vk::SemaphoreCreateFlags flags);
    std::expected<vk::Fence, vk::Result> create_fence(vk::FenceCreateFlags flags);

    [[nodiscard]] vk::Result wait_fence(vk::Fence fence, uint64_t timeout) const;
    [[nodiscard]] vk::Result reset_fence(vk::Fence fence) const;



private:
    bool m_Initialized { false };
    DeletionQueue m_DeletionQueue;

    // Core
    vk::Extent2D m_WindowExtent;
    vk::Instance m_Instance { nullptr };
    vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };
    vk::SurfaceKHR m_Surface { nullptr };
    vk::Device m_Device { nullptr };
    vk::PhysicalDevice m_PhysicalDevice { nullptr };
    VmaAllocator m_Allocator {};

    // Queue
    QueueBundle m_GraphicsQueue {};

    // Sync structure handles
    std::vector<vk::CommandPool> m_CommandPools;
    std::vector<vk::Semaphore> m_Semaphores;
    std::vector<vk::Fence> m_Fences;

    AllocatedImage m_DrawImage {};

    // Swapchain stuff
    SwapchainBundle m_SwapchainBundle;
    vk::Image m_CurrentSwapchainImage { nullptr };
    uint32_t m_CurrentSwapchainImageIndex {};

    // DeletionQueue m_SwapchainDeletionQueue;

    void create_swapchain(uint32_t width, uint32_t height);
    void init_swapchain();
    void destroy_swapchain();
};

}
