#ifndef ENGINE_HPP
#define ENGINE_HPP

namespace Minecraft {

struct FrameData {
    vk::CommandPool CommandPool { nullptr };
    vk::CommandBuffer CommandBuffer { nullptr };
    vk::Semaphore SwapChainSemaphore { nullptr };
    vk::Semaphore RenderSemaphore { nullptr };
    vk::Fence RenderFence { nullptr };
};

class Engine {
public:
    Engine(const uint32_t width, const uint32_t height)
        : m_WindowExtent(width, height)
    {
    }
    ~Engine();

    [[nodiscard]] bool init();
    [[nodiscard]] bool run();

private:
    bool m_IsInitialized = false;
    bool m_Running = false;

    GLFWwindow* m_Window { nullptr };
    vk::Extent2D m_WindowExtent;

    vk::Instance m_Instance { nullptr };
    vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };
    vk::SurfaceKHR m_Surface;

    vk::PhysicalDevice m_PhysicalDevice { nullptr };
    vk::Device m_Device { nullptr };

    vk::SwapchainKHR m_SwapChain { nullptr };
    vk::Format m_SwapChainImageFormat {};
    std::vector<VkImage> m_SwapChainImages;
    std::vector<VkImageView> m_SwapChainImageViews;
    vk::Extent2D m_SwapChainExtent {};

    vk::Queue m_PresentQueue;
    vk::Queue m_GraphicsQueue;
    uint32_t m_GraphicsQueueFamily {};

    vk::RenderPass m_RenderPass { nullptr };
    vk::PipelineLayout m_PipelineLayout { nullptr };
    vk::Pipeline m_Pipeline { nullptr };

    std::vector<vk::Framebuffer> m_SwapChainFramebuffers;

    FrameData m_FrameData;

    [[nodiscard]] bool init_window();
    void init_vulkan();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
    [[nodiscard]] bool create_render_pass();
    [[nodiscard]] bool create_graphics_pipeline();
    [[nodiscard]] bool create_framebuffers();
    [[nodiscard]] bool create_command_pool();
    [[nodiscard]] bool allocate_command_buffer();
    [[nodiscard]] bool record_command_buffer(const vk::CommandBuffer& cmd, uint32_t image_index) const;
    [[nodiscard]] bool create_sync_objects();

    [[nodiscard]] bool draw_frame() const;
};

}

#endif
