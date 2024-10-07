#pragma once

#include "gpu_manager.hpp"

#include <vk_mem_alloc.h>

namespace Minecraft {

struct FrameData {
    vk::CommandPool CommandPool { nullptr };
    vk::CommandBuffer CommandBuffer { nullptr };
    vk::Semaphore SwapChainSemaphore { nullptr };
    vk::Semaphore RenderSemaphore { nullptr };
    vk::Fence RenderFence { nullptr };
    DeletionQueue FrameDeletionQueue;
};

struct Buffer {
    vk::Buffer Handle { nullptr };
    VmaAllocation Allocation;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription get_binding_description()
    {
        return vk::VertexInputBindingDescription {
            0, sizeof(Vertex), vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions()
    {
        return {
            vk::VertexInputAttributeDescription {
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) },

            vk::VertexInputAttributeDescription {
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) }
        };
    }
};

const std::vector<Vertex> vertices = {
    { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
    { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
    { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
};

// TODO Abstractions
/*
 * Application -> main loop and window, makes drawcall on engine
 * Engine ->
 * VulkanContext -> m_Device, m_Swapchain
 */

class Engine {
public:
    explicit Engine(const bool enable_layers) : m_GpuManager(VulkanEngine::GpuManager(enable_layers)) {}
    ~Engine();

    [[nodiscard]] bool init(uint32_t width, uint32_t height);
    [[nodiscard]] bool run();
    bool m_FramebufferResized = false;

private:
    bool m_IsInitialized = false;
    bool m_Running = false;

    DeletionQueue m_MainDeletionQueue;

    int m_FrameNumber { 0 };
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    FrameData& get_current_frame() { return m_Frames[m_FrameNumber % MAX_FRAMES_IN_FLIGHT]; }

    GLFWwindow* m_Window { nullptr };
    VulkanEngine::GpuManager m_GpuManager;

    // TODO lasciare solo device
    vk::SurfaceKHR m_Surface { nullptr };
    vk::PhysicalDevice m_PhysicalDevice { nullptr };
    vk::Device m_Device { nullptr };
    VmaAllocator m_Allocator {};

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

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;

    Buffer m_VertexBuffer {};

    [[nodiscard]] bool init_window(uint32_t width, uint32_t height);
    void init_vulkan();
    void create_swapchain();
    void cleanup_swapchain();
    [[nodiscard]] bool recreate_swapchain();
    [[nodiscard]] bool create_render_pass();
    [[nodiscard]] bool create_graphics_pipeline();
    [[nodiscard]] bool create_framebuffers();
    [[nodiscard]] bool init_commands();
    [[nodiscard]] bool create_vertex_buffer();
    [[nodiscard]] bool record_command_buffer(const vk::CommandBuffer& cmd, uint32_t image_index) const;
    [[nodiscard]] bool create_sync_objects();

    [[nodiscard]] bool draw_frame();
};

}
