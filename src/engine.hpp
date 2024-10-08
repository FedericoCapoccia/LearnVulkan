#pragma once

#include "gpu_manager.hpp"

/*
 * TODO
 *  Create pipelines in GpuManager
 */

/*
 * Notes:
 * - Init glwf window
 * - Init GPUManager:
 * -- Initialize vulkan instance, surface, device, creates swapchain and allocator
 * - Create a Graphics Pipeline (Input -> Vertex Shader -> Fragment Shader -> Rasterizer) // TODO add vertex and index buffers
 * - Setup double buffering
 * -- Request command pool from GPUManager
 * -- Allocate command buffers
 * -- Request Semaphores and Fences
 *
 * - On each frame grab new swapchain image
 * - Reset Command Buffer
 * - Record new command:
 * -- Setup Allocated Image format for color clearing
 * -- Draw background with clear color
 * -- Setup Allocated Image format for geometry drawing
 * -- Draw geometry
 * -- Setup Allocated Image format for transfer src
 * -- Setup Swapchain Image format for transfer dst
 * -- Copy drawn image to swapchain image
 * -- end recording
 * - Send command
 * - Present swapchain image
 */

namespace Minecraft::VkEngine {

struct FrameData {
    vk::CommandPool CommandPool { nullptr };
    vk::CommandBuffer CommandBuffer { nullptr };

    vk::Semaphore SwapChainSemaphore { nullptr };
    vk::Semaphore RenderSemaphore { nullptr };
    vk::Fence RenderFence { nullptr };
};

//const std::vector<Vertex> vertices = {
//    { { 0.0f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
//    { { 0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
//    { { -0.5f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } }
//};

class Engine {
public:
    ~Engine();

    [[nodiscard]] bool init(uint32_t width, uint32_t height);
    [[nodiscard]] bool run();
    bool ResizeRequested = false;

private:
    bool m_IsInitialized = false;
    bool m_Running = false;

    DeletionQueue m_MainDeletionQueue;

    GLFWwindow* m_Window { nullptr };
    GpuManager m_GpuManager {};
    vk::Device m_Device { nullptr };
    DrawImageBundle m_DrawImageBundle {};

    // Resizing
    vk::Extent2D m_DrawExtent {};
    float m_RenderScale = 1.0f;

    // Pipelines
    PipelineBundle m_TrianglePipeline {};

    // Frame stuff
    int m_FrameNumber { 0 };
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> m_Frames;
    FrameData& get_current_frame() { return m_Frames[m_FrameNumber % MAX_FRAMES_IN_FLIGHT]; }

    [[nodiscard]] bool init_window(uint32_t width, uint32_t height);
    void init_vulkan();
    bool init_pipelines();
    bool init_triangle_pipeline();
    [[nodiscard]] bool init_commands();
    [[nodiscard]] bool record_command_buffer(vk::CommandBuffer cmd, vk::Image swapchain_image, vk::Extent2D swapchain_extent);
    [[nodiscard]] bool create_sync_objects();

    [[nodiscard]] bool draw_frame();

    void draw_background(vk::CommandBuffer cmd) const;
    void draw_geometry(vk::CommandBuffer cmd) const;
};

}
