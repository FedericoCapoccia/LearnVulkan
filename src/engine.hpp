#ifndef ENGINE_HPP
#define ENGINE_HPP

namespace Minecraft {

constexpr uint32_t FRAME_OVERLAP = 2;
struct FrameData {
    vk::CommandPool CommandPool;
    vk::CommandBuffer CommandBuffer;
    vk::Semaphore SwapchainSemaphore, RenderSemaphore;
    vk::Fence RenderFence;
};

class Engine {
public:
    Engine(const uint32_t width, const uint32_t height)
        : m_WindowExtent(width, height)
    {
    }
    ~Engine();

    bool init();
    bool run();

    FrameData& get_current_frame() { return m_Frames[m_FrameNumber % FRAME_OVERLAP]; }

private:
    bool m_IsInitialized = false;
    int m_FrameNumber { 0 };
    bool m_Running = false;

    GLFWwindow* m_Window { nullptr };
    vk::Extent2D m_WindowExtent;

    vk::Instance m_Instance { nullptr };
    vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };
    vk::SurfaceKHR m_Surface;

    vk::PhysicalDevice m_PhysicalDevice { nullptr };
    vk::Device m_Device { nullptr };

    vk::SwapchainKHR m_Swapchain { nullptr };
    vk::Format m_SwapchainImageFormat {};
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    vk::Extent2D m_SwapchainExtent {};

    std::array<FrameData, FRAME_OVERLAP> m_Frames;
    vk::Queue m_GraphicsQueue;
    uint32_t m_GraphicsQueueFamily;

    bool init_window();
    void init_vulkan();
    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
    bool init_commands();
    bool init_sync_structures();

    bool draw();
};

}

#endif
