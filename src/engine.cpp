#include "engine.hpp"
#include "gpu_manager.hpp"
#include "helper.hpp"
#include "logger.hpp"
#include "pipeline.hpp"

namespace Minecraft::VkEngine {

Engine::~Engine()
{
    m_GpuManager.wait_idle();
    m_MainDeletionQueue.flush();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

bool Engine::init(const uint32_t width, const uint32_t height)
{
    if (m_IsInitialized) {
        LOG_ERROR("Engine already init");
        return false;
    }

    if (!init_window(width, height))
        return false;

    init_vulkan();

    if (!init_pipelines()) {
        LOG_ERROR("Failed to initialize pipelines");
        return false;
    }

    if (!init_commands()) {
        LOG_ERROR("Failed to initialize command structures");
        return false;
    }

    if (!create_sync_objects()) {
        LOG_ERROR("Failed to create sync objects");
        return false;
    }

    m_IsInitialized = true;
    return true;
}

// static void framebuffer_resize_callback(GLFWwindow* window, const int width, const int height)
//{
//     (void)width;
//     (void)height;
//     const auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
//     engine->m_FramebufferResized = true;
// }

bool Engine::init_window(const uint32_t width, const uint32_t height)
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to init glfw");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_Window = glfwCreateWindow(
        static_cast<int32_t>(width),
        static_cast<int32_t>(height), "Minecraft", nullptr, nullptr);

    if (!m_Window) {
        LOG_ERROR("Failed to create glfw window");
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);
    // glfwSetFramebufferSizeCallback(m_Window, framebuffer_resize_callback);
    return true;
}

void Engine::init_vulkan()
{
    const GpuManagerSpec spec {
        "Minecraft",
        true,
        Logger::debug_callback,
        m_Window
    };

    const auto& [device, draw_image] = m_GpuManager.init(spec);
    m_Device = device;
    m_DrawImageBundle = draw_image;

    m_MainDeletionQueue.push_function("GpuManager", [&] {
        m_GpuManager.destroy();
    });
}

bool Engine::init_pipelines()
{
    if (!init_triangle_pipeline()) {
        LOG_ERROR("Failed to initialize triangle pipeline");
        return false;
    }

    return true;
}

bool Engine::init_triangle_pipeline()
{
    const auto vert_result = VkUtil::load_shader_module("../resources/shaders/basic.vert.spv", m_Device);
    if (!vert_result.has_value()) {
        LOG_ERROR("Failed to create shader module: {}", vert_result.error());
        return false;
    }
    const vk::ShaderModule vertex_module = vert_result.value();

    const auto frag_result = VkUtil::load_shader_module("../resources/shaders/basic.frag.spv", m_Device);
    if (!frag_result.has_value()) {
        LOG_ERROR("Failed to create shader module: {}", frag_result.error());
        return false;
    }
    const vk::ShaderModule fragment_module = frag_result.value();

    const vk::PipelineLayoutCreateInfo pipeline_layout_info = VkInit::pipeline_layout_create_info();
    VK_CHECK(m_Device.createPipelineLayout(&pipeline_layout_info, nullptr, &m_TrianglePipeline.Layout));

    PipelineBuilder builder;
    builder
        .set_shaders(vertex_module, fragment_module)
        .set_input_topology(vk::PrimitiveTopology::eTriangleList)
        .set_polygon_mode(vk::PolygonMode::eFill)
        .set_cull_mode(vk::CullModeFlagBits::eNone, vk::FrontFace::eClockwise)
        .set_multisampling_none()
        .disable_blending()
        .disable_depth_test()
        .set_color_attachment_format(m_DrawImageBundle.Format)
        .set_depth_format(vk::Format::eUndefined);

    const auto pipeline_result = builder.build_pipeline(m_Device, m_TrianglePipeline.Layout);
    if (!pipeline_result.has_value()) {
        LOG_ERROR("Failed to create triangle pipeline: {}", vk::to_string(pipeline_result.error()));
        return false;
    }

    m_TrianglePipeline.Handle = pipeline_result.value();

    m_Device.destroyShaderModule(vertex_module);
    m_Device.destroyShaderModule(fragment_module);

    m_MainDeletionQueue.push_function("Triangle Pipeline", [&] {
        m_Device.destroyPipelineLayout(m_TrianglePipeline.Layout);
        m_Device.destroyPipeline(m_TrianglePipeline.Handle);
    });
    return true;
}

bool Engine::init_commands()
{
    constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    constexpr auto level = vk::CommandBufferLevel::ePrimary;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Command pool
        const auto pres = m_GpuManager.create_command_pool(flags);
        if (!pres.has_value()) {
            VK_CHECK(pres.error());
        }
        m_Frames[i].CommandPool = pres.value();

        // Command Buffer
        const auto bres = m_GpuManager.allocate_command_buffer(m_Frames[i].CommandPool, level);
        if (!bres.has_value()) {
            VK_CHECK(bres.error());
        }
        m_Frames[i].CommandBuffer = bres.value();
    }

    return true;
}

bool Engine::create_sync_objects()
{
    constexpr vk::SemaphoreCreateFlags semaphore_flags {};
    constexpr vk::FenceCreateFlags fence_flags { vk::FenceCreateFlagBits::eSignaled };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

        const auto fence_res = m_GpuManager.create_fence(fence_flags);
        if (!fence_res.has_value()) {
            VK_CHECK(fence_res.error());
        }
        m_Frames[i].RenderFence = fence_res.value();

        const auto semaphore1_res = m_GpuManager.create_semaphore(semaphore_flags);
        if (!semaphore1_res.has_value()) {
            VK_CHECK(semaphore1_res.error());
        }
        m_Frames[i].SwapChainSemaphore = semaphore1_res.value();


        const auto semaphore2_res = m_GpuManager.create_semaphore(semaphore_flags);
        if (!semaphore2_res.has_value()) {
            VK_CHECK(semaphore2_res.error());
        }
        m_Frames[i].RenderSemaphore = semaphore2_res.value();
    }

    return true;
}

void Engine::draw_background(const vk::CommandBuffer cmd) const
{
    constexpr vk::ClearColorValue clear_value = { 0.0f, 0.0f, 0.0f, 1.0f };
    const vk::ImageSubresourceRange clear_range = VkUtil::image_subresource_range(vk::ImageAspectFlagBits::eColor);

    cmd.clearColorImage(m_DrawImageBundle.Image, vk::ImageLayout::eGeneral, &clear_value, 1, &clear_range);
}

void Engine::draw_geometry(const vk::CommandBuffer cmd) const
{
    const vk::RenderingAttachmentInfo color_attachment = VkInit::attachment_info(m_DrawImageBundle.ImageView, nullptr, vk::ImageLayout::eColorAttachmentOptimal);

    const vk::RenderingInfo rendering_info = VkInit::rendering_info(m_DrawExtent, &color_attachment, nullptr);

    cmd.beginRendering(&rendering_info);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_TrianglePipeline.Handle);

    vk::Viewport viewport {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = static_cast<float>(m_DrawExtent.width);
    viewport.height = static_cast<float>(m_DrawExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;
    cmd.setViewport(0, 1, &viewport);

    vk::Rect2D scissor {};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = m_DrawExtent.width;
    scissor.extent.height = m_DrawExtent.height;
    cmd.setScissor(0, 1, &scissor);

    cmd.draw(3, 1, 0, 0);
    cmd.endRendering();
}

bool Engine::record_command_buffer(const vk::CommandBuffer cmd, const vk::Image swapchain_image, const vk::Extent2D swapchain_extent)
{
    constexpr auto flags {
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    constexpr vk::CommandBufferBeginInfo create_info {
        flags,
        nullptr
    };

    m_DrawExtent.width = m_DrawImageBundle.Extent.width;
    m_DrawExtent.height = m_DrawImageBundle.Extent.height;

    VK_CHECK(cmd.begin(create_info));

    VkUtil::transition_image(cmd, m_DrawImageBundle.Image, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    draw_background(cmd);

    VkUtil::transition_image(cmd, m_DrawImageBundle.Image, vk::ImageLayout::eGeneral, vk::ImageLayout::eColorAttachmentOptimal);

    draw_geometry(cmd);

    // transition the draw image and the swapchain image into their correct transfer layouts
    VkUtil::transition_image(cmd, m_DrawImageBundle.Image, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal);
    VkUtil::transition_image(cmd, swapchain_image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

    VkUtil::copy_image_to_image(cmd,
        m_DrawImageBundle.Image, swapchain_image,
        m_DrawExtent, swapchain_extent);

    VkUtil::transition_image(cmd, swapchain_image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR);

    VK_CHECK(cmd.end());

    return true;
}

bool Engine::draw_frame()
{
    VK_CHECK(m_GpuManager.wait_fence(get_current_frame().RenderFence, UINT64_MAX));

    const auto res = m_GpuManager.get_next_swapchain_image(get_current_frame().SwapChainSemaphore, UINT64_MAX);
    if (!res.has_value()) {
        LOG_ERROR("Failed to acquire swap chain image");
        return false;
    }

    const vk::Image swapchain_image = res.value();
    const vk::Extent2D swapchain_extent = m_GpuManager.get_swapchain_extent();

    VK_CHECK(m_GpuManager.reset_fence(get_current_frame().RenderFence));

    const vk::CommandBuffer& cmd = get_current_frame().CommandBuffer;
    VK_CHECK(cmd.reset());

    if (!record_command_buffer(cmd, swapchain_image, swapchain_extent)) {
        LOG_ERROR("Failed to record on the command buffer");
        return false;
    }

    vk::CommandBufferSubmitInfo cmd_info {
        cmd, 0
    };

    vk::SemaphoreSubmitInfo wait_info {
        get_current_frame().SwapChainSemaphore, 1,
        vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput
    };

    vk::SemaphoreSubmitInfo signal_info {
        get_current_frame().RenderSemaphore, 1,
        vk::PipelineStageFlagBits2::eAllGraphics
    };

    const vk::SubmitInfo2 submit_info {
        {},
        1, &wait_info,
        1, &cmd_info,
        1, &signal_info
    };

    VK_CHECK(m_GpuManager.submit_to_queue(submit_info, get_current_frame().RenderFence));
    VK_CHECK(m_GpuManager.present(1, &get_current_frame().RenderSemaphore));

    m_FrameNumber++;
    return true;
}

bool Engine::run()
{
    m_Running = true;
    LOG("Engine started");
    while (m_Running) {
        glfwPollEvents();

        if (!draw_frame()) {
            LOG_ERROR("Error in frame");
        }

        m_Running = !glfwWindowShouldClose(m_Window);
        // m_Running = false;
    }

    LOG("Engine stopped");
    return true;
}

}