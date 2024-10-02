#ifndef ENGINE_HPP
#define ENGINE_HPP

#include "logger.hpp"

namespace Minecraft {

struct WindowBundle {
    GLFWwindow* GlfwWindow;
    int32_t Width;
    int32_t Height;
};

class Engine {
public:
    WindowBundle Window {};
    bool init();
    Engine(int32_t width, int32_t height);
    ~Engine();

    [[nodiscard]] bool is_running() const { return m_Running; }
    void start()
    {
        LOG("Engine started");
        m_Running = true;
    }
    void stop()
    {
        LOG("Engine stopped");
        m_Running = false;
    }

private:
    bool m_IsInitialized = false;
    bool m_Running = false;

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

    void init_window();
    void init_context();
    void create_swapchain(int32_t width, int32_t height);
    void destroy_swapchain();
};

}

#endif
