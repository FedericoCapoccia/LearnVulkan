#pragma once

#include <vk_mem_alloc.h>

namespace Minecraft {

struct DeletionQueue {
    std::deque<std::function<void()>> Deletors;

    void push_function(std::function<void()>&& function) {
        Deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto & Deletor : std::ranges::reverse_view(Deletors)) {
            Deletor(); //call functors
        }
        Deletors.clear();
    }
};

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

    int m_FrameNumber { 0 };
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    FrameData& get_current_frame() { return m_Frames[m_FrameNumber % MAX_FRAMES_IN_FLIGHT]; }

    GLFWwindow* m_Window { nullptr };

    VmaAllocator m_Allocator {};
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
