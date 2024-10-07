#pragma once

#include "gpu_manager.hpp"

#include <vk_mem_alloc.h>

namespace Minecraft::VkEngine {

struct FrameData {
    vk::CommandPool CommandPool { nullptr };
    vk::CommandBuffer CommandBuffer { nullptr };
    vk::Semaphore SwapChainSemaphore { nullptr };
    vk::Semaphore RenderSemaphore { nullptr };
    vk::Fence RenderFence { nullptr };
    DeletionQueue FrameDeletionQueue;
};

const std::vector<Vertex> vertices = {
    { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

class Engine {
public:
    ~Engine();

    [[nodiscard]] bool init(uint32_t width, uint32_t height);
    [[nodiscard]] bool run();
    bool m_FramebufferResized = false;

private:
    bool m_IsInitialized = false;
    bool m_Running = false;

    DeletionQueue m_MainDeletionQueue;

    [[nodiscard]] SwapchainSpec get_default_swapchain_spec() const
    {
        return SwapchainSpec {
            m_Window,
            { vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear },
            vk::PresentModeKHR::eFifo,
            vk::ImageUsageFlagBits::eTransferDst,
            vk::CompositeAlphaFlagBitsKHR::eOpaque
        };
    }

    GLFWwindow* m_Window { nullptr };
    GpuManager m_GpuManager {};

    vk::Device m_Device { nullptr };
    QueueBundle m_GraphicsQueue {};

    SwapchainBundle m_SwapchainBundle;

    int m_FrameNumber { 0 };
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
    FrameData& get_current_frame() { return m_Frames[m_FrameNumber % MAX_FRAMES_IN_FLIGHT]; }

    vk::Buffer m_VertexBufferHandle {};

    [[nodiscard]] bool init_window(uint32_t width, uint32_t height);
    void init_vulkan();
    [[nodiscard]] bool init_commands();
    [[nodiscard]] bool record_command_buffer(const vk::CommandBuffer& cmd, uint32_t image_index) const;
    [[nodiscard]] bool create_sync_objects();

    [[nodiscard]] bool draw_frame();
};

}
