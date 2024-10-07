#include "engine.hpp"
#include "gpu_manager.hpp"
#include "logger.hpp"

// Important for VMA, better to sync with GpuManager::InstanceSpec::ApiVersion
#define VMA_VULKAN_VERSION 1003000

namespace Minecraft::VkEngine {

constexpr bool enable_validation_layers = true;

bool Engine::init(const uint32_t width, const uint32_t height)
{
    if (m_IsInitialized) {
        LOG_ERROR("Graphics Manager already initialized.");
        return false;
    }

    if (!init_window(width, height))
        return false;

    init_vulkan();

    if (!create_graphics_pipeline()) {
        LOG_ERROR("Failed to create the graphics pipeline");
        return false;
    }

    const auto res = m_GpuManager.create_vertex_buffer(vertices);

    if (!res.has_value()) {
        LOG_ERROR("{}", res.error());
    }

    m_VertexBufferHandle = res.value();

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

static void framebuffer_resize_callback(GLFWwindow* window, const int width, const int height)
{
    (void)width;
    (void)height;
    const auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    engine->m_FramebufferResized = true;
}

bool Engine::init_window(const uint32_t width, const uint32_t height)
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to init glfw");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_Window = glfwCreateWindow(
        static_cast<int32_t>(width),
        static_cast<int32_t>(height), "Minecraft", nullptr, nullptr);

    if (!m_Window) {
        LOG_ERROR("Failed to create glfw window");
        return false;
    }

    glfwSetWindowUserPointer(m_Window, this);
    glfwSetFramebufferSizeCallback(m_Window, framebuffer_resize_callback);
    return true;
}

void Engine::init_vulkan()
{
    const InstanceSpec instance_spec {
        "Minecraft",
        { 1, 3, 0 },
        true,
        Logger::debug_callback,
        {}
    };

    const DeviceSpec device_spec {
        { 1, 3 },
        false,
        false
    };

    const GpuManagerSpec spec {
        instance_spec,
        device_spec,
        get_default_swapchain_spec()
    };

    m_SwapchainBundle = m_GpuManager.init(spec);

    m_MainDeletionQueue.push_function("Gpu Manager", [&] {
        m_GpuManager.destroy();
    });

    m_Device = m_GpuManager.device();

    m_PresentQueue = m_GpuManager.get_queue(vkb::QueueType::present, false);
    m_GraphicsQueue = m_GpuManager.get_queue(vkb::QueueType::graphics, false);
}

static std::vector<char> read_file(const std::string& filepath)
{
    // ate starts reading at the end of file
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        fmt::println(std::cerr, "Unable to open shader file: {}", filepath);
    }

    const std::streamsize file_size = file.tellg();
    std::vector<char> buffer;
    buffer.resize(file_size);

    file.seekg(0); // return at beginning of file
    file.read(buffer.data(), file_size);

    file.close();
    return buffer;
}

bool Engine::create_graphics_pipeline()
{
#pragma region shaders
    // Vertex Shader
    const auto vert_code = read_file("../resources/shaders/basic.vert.spv");
    const auto vertex_create_info = vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(),
        vert_code.size(),
        reinterpret_cast<const uint32_t*>(vert_code.data()));

    const auto [vert_result, vert_shader_module] = m_Device.createShaderModule(vertex_create_info);
    VK_CHECK(vert_result);

    const auto vert_shader_stage_info = vk::PipelineShaderStageCreateInfo(
        {},
        vk::ShaderStageFlagBits::eVertex,
        vert_shader_module, "main");

    // Fragment Shader
    const auto frag_code = read_file("../resources/shaders/basic.frag.spv");
    const auto fragment_create_info = vk::ShaderModuleCreateInfo(
        vk::ShaderModuleCreateFlags(),
        frag_code.size(),
        reinterpret_cast<const uint32_t*>(frag_code.data()));

    const auto [frag_result, frag_shader_module] = m_Device.createShaderModule(fragment_create_info);
    VK_CHECK(frag_result);

    const auto frag_shader_stage_info = vk::PipelineShaderStageCreateInfo(
        {},
        vk::ShaderStageFlagBits::eFragment,
        frag_shader_module, "main");

    const vk::PipelineShaderStageCreateInfo shader_stages[] = {
        vert_shader_stage_info, frag_shader_stage_info
    };

    const auto binding_description = Vertex::get_binding_description();
    const auto attribute_descriptions = Vertex::get_attribute_descriptions();

    const auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo({},
        1, &binding_description,
        attribute_descriptions.size(), attribute_descriptions.data());

#pragma endregion

#pragma region Pipeline
    constexpr auto rasterizer = vk::PipelineRasterizationStateCreateInfo({},
        vk::False,
        vk::False,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eBack,
        vk::FrontFace::eClockwise,
        vk::False,
        0.0f,
        0.0f,
        0.0f,
        1.0f);

    constexpr auto multisampling = vk::PipelineMultisampleStateCreateInfo({},
        vk::SampleCountFlagBits::e1,
        vk::False,
        1.0f,
        nullptr,
        vk::False,
        vk::False);

    constexpr auto color_write_mask = vk::ColorComponentFlagBits::eR
        | vk::ColorComponentFlagBits::eG
        | vk::ColorComponentFlagBits::eB
        | vk::ColorComponentFlagBits::eA;

    const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState(
        vk::False,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eOne,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        color_write_mask);

    const auto color_blending = vk::PipelineColorBlendStateCreateInfo({},
        vk::False,
        vk::LogicOp::eCopy,
        1,
        &color_blend_attachment,
        { 0.0f, 0.0f, 0.0f, 0.0f });

    constexpr auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo({},
        vk::PrimitiveTopology::eTriangleList,
        vk::False);

    // Dynamic information that can be changed without dropping the pipeline
    constexpr std::array dynamic_states = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const auto dynamic_state = vk::PipelineDynamicStateCreateInfo({},
        dynamic_states.size(), dynamic_states.data());

    constexpr auto viewport_state = vk::PipelineViewportStateCreateInfo({},
        1, {},
        1, {});

    constexpr auto pipeline_layout_info = vk::PipelineLayoutCreateInfo({},
        0, nullptr,
        0, nullptr);

    VK_CHECK(m_Device.createPipelineLayout(&pipeline_layout_info, nullptr, &m_PipelineLayout));

    m_MainDeletionQueue.push_function("Pipeline layout", [&] {
        m_Device.destroyPipelineLayout(m_PipelineLayout);
    });

    const auto pipeline_info = vk::GraphicsPipelineCreateInfo({},
        2, shader_stages,
        &vertex_input_info,
        &input_assembly,
        {},
        &viewport_state,
        &rasterizer,
        &multisampling,
        nullptr,
        &color_blending,
        &dynamic_state,
        m_PipelineLayout,
        m_GpuManager.render_pass(),
        0,
        VK_NULL_HANDLE,
        -1);

    const auto [pipeline_res, pipeline] = m_Device.createGraphicsPipeline(VK_NULL_HANDLE, pipeline_info);

    if (pipeline_res != vk::Result::eSuccess && pipeline_res != vk::Result::ePipelineCompileRequiredEXT) {
        LOG_ERROR("Vulkan error: {}", vk::to_string(pipeline_res));
        return false;
    }

    m_Pipeline = pipeline;

    m_MainDeletionQueue.push_function("Pipeline", [&] {
        m_Device.destroyPipeline(m_Pipeline);
    });
#pragma endregion

    m_Device.destroyShaderModule(vert_shader_module);
    m_Device.destroyShaderModule(frag_shader_module);
    return true;
}

bool Engine::init_commands()
{
    constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    const auto pool_info = vk::CommandPoolCreateInfo(flags, m_GraphicsQueue.FamilyIndex);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(m_Device.createCommandPool(&pool_info, nullptr, &m_Frames[i].CommandPool));

        const auto allocate_info = vk::CommandBufferAllocateInfo(
            m_Frames[i].CommandPool, vk::CommandBufferLevel::ePrimary, 1);

        VK_CHECK(m_Device.allocateCommandBuffers(&allocate_info, &m_Frames[i].CommandBuffer));
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
        VK_CHECK(m_Device.createFence(&fence_create_info, nullptr, &m_Frames[i].RenderFence));
        VK_CHECK(m_Device.createSemaphore(&semaphore_create_info, nullptr, &m_Frames[i].SwapChainSemaphore));
        VK_CHECK(m_Device.createSemaphore(&semaphore_create_info, nullptr, &m_Frames[i].RenderSemaphore));
    }

    return true;
}

bool Engine::record_command_buffer(const vk::CommandBuffer& cmd, const uint32_t image_index) const
{
    vk::CommandBufferUsageFlagBits flags {};
    VK_CHECK(cmd.begin({ flags }));

    const vk::Rect2D scissor({ 0, 0 }, m_SwapchainBundle.Extent);

    vk::ClearValue clear_color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    const vk::RenderPassBeginInfo render_pass_info {
        m_GpuManager.render_pass(),
        m_SwapchainBundle.Framebuffers[image_index],
        scissor,
        1, &clear_color
    };

    cmd.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);

    // TODO Vertex Buffer things
    const vk::Buffer vertexBuffers[] = { m_VertexBufferHandle };
    // ReSharper disable once CppRedundantZeroInitializerInAggregateInitialization
    constexpr vk::DeviceSize offsets[] = { 0 };

    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    // Scissor and viewport are bound here because are marked as dynamic on pipeline creation
    cmd.setScissor(0, 1, &scissor);

    const vk::Viewport viewport(0.0f, 0.0f,
        static_cast<float>(m_SwapchainBundle.Extent.width),
        static_cast<float>(m_SwapchainBundle.Extent.height),
        0.0f, 1.0f);
    cmd.setViewport(0, 1, &viewport);

    cmd.draw(vertices.size(), 1, 0, 0);

    cmd.endRenderPass();

    VK_CHECK(cmd.end());
    return true;
}

bool Engine::draw_frame()
{
    VK_CHECK(m_Device.waitForFences(1, &get_current_frame().RenderFence, vk::True, UINT64_MAX));

    uint32_t image_index;
    vk::Result result = m_Device.acquireNextImageKHR(
        m_SwapchainBundle.Swapchain, UINT64_MAX, get_current_frame().SwapChainSemaphore, VK_NULL_HANDLE, &image_index);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        m_SwapchainBundle = m_GpuManager.recreate_swapchain(get_default_swapchain_spec());
        return true;
    }

    if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
        LOG_ERROR("Failed to acquire swap chain image");
        return false;
    }

    VK_CHECK(m_Device.resetFences(1, &get_current_frame().RenderFence));

    const vk::CommandBuffer& cmd = get_current_frame().CommandBuffer;
    VK_CHECK(cmd.reset());
    if (!record_command_buffer(cmd, image_index)) {
        LOG_ERROR("Failed to record on the command buffer");
        return false;
    }

    vk::Semaphore wait_semaphores[] = { get_current_frame().SwapChainSemaphore };
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signal_semaphores[] = { get_current_frame().RenderSemaphore };

    const vk::SubmitInfo submit_info {
        1, wait_semaphores, wait_stages,
        1, &cmd,
        1, signal_semaphores
    };

    VK_CHECK(m_GraphicsQueue.Queue.submit(1, &submit_info, get_current_frame().RenderFence));

    vk::SwapchainKHR swapchains[] = { m_SwapchainBundle.Swapchain };

    const vk::PresentInfoKHR present_info {
        1, signal_semaphores,
        1, swapchains, &image_index,
        nullptr
    };

    result = m_PresentQueue.Queue.presentKHR(present_info);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        m_SwapchainBundle = m_GpuManager.recreate_swapchain(get_default_swapchain_spec());
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
    VK_CHECK(m_Device.waitIdle());
    LOG("Engine stopped");
    return true;
}

Engine::~Engine()
{
    m_GpuManager.cleanup_swapchain();

    if (m_Frames[0].CommandPool) {
        // Sync Objects
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_Device.destroyCommandPool(m_Frames[i].CommandPool);

            m_Device.destroyFence(m_Frames[i].RenderFence);
            m_Device.destroySemaphore(m_Frames[i].RenderSemaphore);
            m_Device.destroySemaphore(m_Frames[i].SwapChainSemaphore);
        }
    }

    m_MainDeletionQueue.flush();

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}