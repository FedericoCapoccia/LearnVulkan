#pragma once
#include "types.hpp"
#include <vk_mem_alloc.h>

namespace Minecraft::VkEngine {

class GpuManager {
public:
    GpuManager() { fmt::println("Gpu manager created"); }
    ResourcesBundle init(const GpuManagerSpec& spec);

    [[nodiscard]] uint32_t get_graphics_queue_index() const { return m_GraphicsQueue.FamilyIndex; }
    std::expected<vk::Image, vk::Result> get_next_swapchain_image(vk::Semaphore swapchain_semaphore, uint64_t timeout);
    [[nodiscard]] vk::Extent2D get_swapchain_extent() const { return m_SwapchainBundle.Extent; }

    [[nodiscard]] vk::Result submit_to_queue(const vk::SubmitInfo2& submit_info2, const vk::Fence render_fence) const
    {
        return m_GraphicsQueue.Queue.submit2(1, &submit_info2, render_fence);
    };

    vk::Result present(uint32_t semaphores_count, vk::Semaphore* semaphores);

    void destroy()
    {
        fmt::println("GpuManager destructor");
        destroy_swapchain();
        m_DeletionQueue.flush();
        m_Initialized = false;
    }

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
