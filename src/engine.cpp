#include "engine.hpp"
#include "gpu_manager.hpp"
#include "logger.hpp"

namespace Minecraft::VkEngine {

constexpr bool enable_validation_layers = true;

bool Engine::init(const uint32_t width, const uint32_t height)
{
    if (m_IsInitialized) {
        LOG_ERROR("Engine already init");
        return false;
    }

    if (!init_window(width, height))
        return false;

    init_vulkan();

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

//static void framebuffer_resize_callback(GLFWwindow* window, const int width, const int height)
//{
//    (void)width;
//    (void)height;
//    const auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
//    engine->m_FramebufferResized = true;
//}

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
    //glfwSetFramebufferSizeCallback(m_Window, framebuffer_resize_callback);
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

    m_ResourcesBundle = m_GpuManager.init(spec);

    m_MainDeletionQueue.push_function("GpuManager", [&] {
        m_GpuManager.destroy();
    });
}

//static std::vector<char> read_file(const std::string& filepath)
//{
//    // ate starts reading at the end of file
//    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
//
//    if (!file.is_open()) {
//        fmt::println(std::cerr, "Unable to open shader file: {}", filepath);
//    }
//
//    const std::streamsize file_size = file.tellg();
//    std::vector<char> buffer;
//    buffer.resize(file_size);
//
//    file.seekg(0); // return at beginning of file
//    file.read(buffer.data(), file_size);
//
//    file.close();
//    return buffer;
//}

//bool Engine::create_graphics_pipeline()
//{
//#pragma region shaders
//    // Vertex Shader
//    const auto vert_code = read_file("../resources/shaders/basic.vert.spv");
//    const auto vertex_create_info = vk::ShaderModuleCreateInfo(
//        vk::ShaderModuleCreateFlags(),
//        vert_code.size(),
//        reinterpret_cast<const uint32_t*>(vert_code.data()));
//
//    const auto [vert_result, vert_shader_module] = m_Device.createShaderModule(vertex_create_info);
//    VK_CHECK(vert_result);
//
//    const auto vert_shader_stage_info = vk::PipelineShaderStageCreateInfo(
//        {},
//        vk::ShaderStageFlagBits::eVertex,
//        vert_shader_module, "main");
//
//    // Fragment Shader
//    const auto frag_code = read_file("../resources/shaders/basic.frag.spv");
//    const auto fragment_create_info = vk::ShaderModuleCreateInfo(
//        vk::ShaderModuleCreateFlags(),
//        frag_code.size(),
//        reinterpret_cast<const uint32_t*>(frag_code.data()));
//
//    const auto [frag_result, frag_shader_module] = m_Device.createShaderModule(fragment_create_info);
//    VK_CHECK(frag_result);
//
//    const auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo(
//        {},
//        vk::ShaderStageFlagBits::eFragment,
//        frag_shader_module, "main");
//
//    const vk::PipelineShaderStageCreateInfo shader_stages[] = {
//        vert_shader_stage_info, frag_shader_stage_info
//    };
//
//    const auto binding_description = Vertex::get_binding_description();
//    const auto attribute_descriptions = Vertex::get_attribute_descriptions();
//
//    const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo({},
//        1, &binding_description,
//        attribute_descriptions.size(), attribute_descriptions.data());
//
//#pragma endregion
//
//#pragma region Pipeline
//    constexpr auto rasterizer = vk::PipelineRasterizationStateCreateInfo({},
//        vk::False,
//        vk::False,
//        vk::PolygonMode::eFill,
//        vk::CullModeFlagBits::eBack,
//        vk::FrontFace::eClockwise,
//        vk::False,
//        0.0f,
//        0.0f,
//        0.0f,
//        1.0f);
//
//    constexpr auto multisampling = vk::PipelineMultisampleStateCreateInfo({},
//        vk::SampleCountFlagBits::e1,
//        vk::False,
//        1.0f,
//        nullptr,
//        vk::False,
//        vk::False);
//
//    constexpr auto color_write_mask = vk::ColorComponentFlagBits::eR
//        | vk::ColorComponentFlagBits::eG
//        | vk::ColorComponentFlagBits::eB
//        | vk::ColorComponentFlagBits::eA;
//
//    const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState(
//        vk::False,
//        vk::BlendFactor::eOne,
//        vk::BlendFactor::eZero,
//        vk::BlendOp::eAdd,
//        vk::BlendFactor::eOne,
//        vk::BlendFactor::eZero,
//        vk::BlendOp::eAdd,
//        color_write_mask);
//
//    const auto color_blending = vk::PipelineColorBlendStateCreateInfo({},
//        vk::False,
//        vk::LogicOp::eCopy,
//        1,
//        &color_blend_attachment,
//        { 0.0f, 0.0f, 0.0f, 0.0f });
//
//    constexpr auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo({},
//        vk::PrimitiveTopology::eTriangleList,
//        vk::False);
//
//    // Dynamic information that can be changed without dropping the pipeline
//    constexpr std::array dynamic_states = {
//        vk::DynamicState::eViewport,
//        vk::DynamicState::eScissor
//    };
//
//    // ReSharper disable once CppVariableCanBeMadeConstexpr
//    const auto dynamic_state = vk::PipelineDynamicStateCreateInfo({},
//        dynamic_states.size(), dynamic_states.data());
//
//    constexpr auto viewport_state = vk::PipelineViewportStateCreateInfo({},
//        1, {},
//        1, {});
//
//    constexpr auto pipeline_layout_info = vk::PipelineLayoutCreateInfo({},
//        0, nullptr,
//        0, nullptr);
//
//    VK_CHECK(m_Device.createPipelineLayout(&pipeline_layout_info, nullptr, &m_PipelineLayout));
//
//    m_MainDeletionQueue.push_function("Pipeline layout", [&] {
//        m_Device.destroyPipelineLayout(m_PipelineLayout);
//    });
//
//    const auto pipeline_info = vk::GraphicsPipelineCreateInfo({},
//        2, shader_stages,
//        &vertex_input_info,
//        &input_assembly,
//        {},
//        &viewport_state,
//        &rasterizer,
//        &multisampling,
//        nullptr,
//        &color_blending,
//        &dynamic_state,
//        m_PipelineLayout,
//        m_GpuManager.render_pass(),
//        0,
//        VK_NULL_HANDLE,
//        -1);
//
//    const auto [pipeline_res, pipeline] = m_Device.createGraphicsPipeline(VK_NULL_HANDLE, pipeline_info);
//
//    if (pipeline_res != vk::Result::eSuccess && pipeline_res != vk::Result::ePipelineCompileRequiredEXT) {
//        LOG_ERROR("Vulkan error: {}", vk::to_string(pipeline_res));
//        return false;
//    }
//
//    m_Pipeline = pipeline;
//
//    m_MainDeletionQueue.push_function("Pipeline", [&] {
//        m_Device.destroyPipeline(m_Pipeline);
//    });
//#pragma endregion
//
//    m_Device.destroyShaderModule(vert_shader_module);
//    m_Device.destroyShaderModule(frag_shader_module);
//    return true;
//}

bool Engine::init_commands()
{
    constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    const auto pool_info = vk::CommandPoolCreateInfo(flags, m_ResourcesBundle.GraphicsQueue.FamilyIndex);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(m_ResourcesBundle.DeviceHandle.createCommandPool(&pool_info, nullptr, &m_Frames[i].CommandPool));

        const auto allocate_info = vk::CommandBufferAllocateInfo(
            m_Frames[i].CommandPool, vk::CommandBufferLevel::ePrimary, 1);

        VK_CHECK(m_ResourcesBundle.DeviceHandle.allocateCommandBuffers(&allocate_info, &m_Frames[i].CommandBuffer));
    }

    return true;
}

bool Engine::create_sync_objects()
{
    constexpr vk::SemaphoreCreateInfo semaphore_create_info {};
    constexpr vk::FenceCreateInfo fence_create_info {
        vk::FenceCreateFlagBits::eSignaled
    };

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(m_ResourcesBundle.DeviceHandle.createFence(&fence_create_info, nullptr, &m_Frames[i].RenderFence));
        VK_CHECK(m_ResourcesBundle.DeviceHandle.createSemaphore(&semaphore_create_info, nullptr, &m_Frames[i].SwapChainSemaphore));
        VK_CHECK(m_ResourcesBundle.DeviceHandle.createSemaphore(&semaphore_create_info, nullptr, &m_Frames[i].RenderSemaphore));
    }

    return true;
}

static vk::ImageSubresourceRange image_subresource_range(vk::ImageAspectFlags aspect_mask)
{
    return {
        aspect_mask,
        0,
        vk::RemainingMipLevels,
        0,
        vk::RemainingArrayLayers
    };
}

static void transition_image(const vk::CommandBuffer& cmd, const vk::Image& image, const vk::ImageLayout& src_layout, const vk::ImageLayout& dst_layout)
{
    // ReSharper disable once CppDFAConstantConditions
    const vk::ImageAspectFlags aspect_mask = dst_layout == vk::ImageLayout::eDepthAttachmentOptimal
        ? vk::ImageAspectFlagBits::eDepth
        : vk::ImageAspectFlagBits::eColor;

    vk::ImageMemoryBarrier2 image_barrier {
        vk::PipelineStageFlagBits2::eAllCommands, // TODO inefficient https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples
        vk::AccessFlagBits2::eMemoryWrite,
        vk::PipelineStageFlagBits2::eAllCommands,
        vk::AccessFlagBits2::eMemoryWrite | vk::AccessFlagBits2::eMemoryRead,
        src_layout, dst_layout,
        {}, {},
        image, image_subresource_range(aspect_mask)
    };

    const vk::DependencyInfo dependency_info {
        {},
        {}, {},
        {}, {},
        1, &image_barrier
    };

    cmd.pipelineBarrier2(dependency_info);
}

bool Engine::record_command_buffer(const vk::CommandBuffer& cmd, const uint32_t image_index) const
{
    constexpr auto flags {
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    constexpr vk::CommandBufferBeginInfo create_info {
        flags,
        nullptr
    };

    VK_CHECK(cmd.begin(create_info));

    //m_SwapchainBundle.DrawExtent =

    transition_image(cmd, m_ResourcesBundle.Swapchain.Images[image_index],
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    constexpr vk::ClearColorValue clear_value = { 0.0f, 1.0f, 0.0f, 1.0f };
    const vk::ImageSubresourceRange clear_range = image_subresource_range(vk::ImageAspectFlagBits::eColor);

    cmd.clearColorImage(m_ResourcesBundle.Swapchain.Images[image_index],
        vk::ImageLayout::eGeneral, &clear_value, 1, &clear_range);

    transition_image(cmd, m_ResourcesBundle.Swapchain.Images[image_index],
        vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR);

    VK_CHECK(cmd.end());

    return true;
}

bool Engine::draw_frame()
{
    VK_CHECK(m_ResourcesBundle.DeviceHandle.waitForFences(1, &get_current_frame().RenderFence, vk::True, UINT64_MAX));

    get_current_frame().FrameDeletionQueue.flush();

    uint32_t image_index;
    vk::Result result = m_ResourcesBundle.DeviceHandle.acquireNextImageKHR(
        m_ResourcesBundle.Swapchain.Handle, UINT64_MAX, get_current_frame().SwapChainSemaphore, VK_NULL_HANDLE, &image_index);

    //if (result == vk::Result::eErrorOutOfDateKHR) {
    //    m_ResourcesBundle.Swapchain = m_GpuManager.recreate_swapchain(get_default_swapchain_spec());
    //    return true;
    //}

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        LOG_ERROR("Failed to acquire swap chain image");
        return false;
    }

    VK_CHECK(m_ResourcesBundle.DeviceHandle.resetFences(1, &get_current_frame().RenderFence));

    const vk::CommandBuffer& cmd = get_current_frame().CommandBuffer;
    VK_CHECK(cmd.reset());

    if (!record_command_buffer(cmd, image_index)) {
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

    VK_CHECK(m_ResourcesBundle.GraphicsQueue.Queue.submit2(1, &submit_info, get_current_frame().RenderFence));

    const vk::PresentInfoKHR present_info {
        1,
        &get_current_frame().RenderSemaphore,
        1,
        &m_ResourcesBundle.Swapchain.Handle,
        &image_index,
    };

    result = m_ResourcesBundle.GraphicsQueue.Queue.presentKHR(present_info);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_FramebufferResized) {
        //m_FramebufferResized = false;
        //m_SwapchainBundle = m_GpuManager.recreate_swapchain(get_default_swapchain_spec());
    } else if (result != vk::Result::eSuccess) {
        LOG_ERROR("failed to present swap chain image!");
        return false;
    }

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
    VK_CHECK(m_ResourcesBundle.DeviceHandle.waitIdle());
    LOG("Engine stopped");
    return true;
}

Engine::~Engine()
{
    // TODO incorporate in dequeue
    if (m_Frames[0].CommandPool) {
        // Sync Objects
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_ResourcesBundle.DeviceHandle.destroyCommandPool(m_Frames[i].CommandPool);

            m_ResourcesBundle.DeviceHandle.destroyFence(m_Frames[i].RenderFence);
            m_ResourcesBundle.DeviceHandle.destroySemaphore(m_Frames[i].RenderSemaphore);
            m_ResourcesBundle.DeviceHandle.destroySemaphore(m_Frames[i].SwapChainSemaphore);

            m_Frames[i].FrameDeletionQueue.flush();
        }
    }

    m_GpuManager.destroy_swapchain();
    m_MainDeletionQueue.flush();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}