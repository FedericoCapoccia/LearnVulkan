#include "engine.hpp"
#include "logger.hpp"

namespace Minecraft {

constexpr bool enable_validation_layers = true;

bool Engine::init()
{
    if (m_IsInitialized) {
        LOG_ERROR("Graphics Manager already initialized.");
        return false;
    }

    if (!init_window())
        return false;

    init_vulkan();
    create_swapchain(m_WindowExtent.width, m_WindowExtent.width);

    if (!create_render_pass()) {
        LOG_ERROR("Failed to create render pass");
        return false;
    }

    if (!create_graphics_pipeline()) {
        LOG_ERROR("Failed to create the graphics pipeline");
        return false;
    }

    if (!create_framebuffers()) {
        LOG_ERROR("Failed to create framebuffers");
        return false;
    }

    if (!create_command_pool()) {
        LOG_ERROR("Failed to create command pool");
        return false;
    }

    if (!allocate_command_buffer()) {
        LOG_ERROR("Failed to allocate command buffer");
        return false;
    }

    if (!create_sync_objects()) {
        LOG_ERROR("Failed to create sync objects");
        return false;
    }

    m_IsInitialized = true;
    return true;
}

bool Engine::init_window()
{
    if (!glfwInit()) {
        LOG_ERROR("Failed to init glfw");
        return false;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    m_Window = glfwCreateWindow(
        static_cast<int>(m_WindowExtent.width),
        static_cast<int>(m_WindowExtent.height),
        "Minecraft", nullptr, nullptr);

    if (!m_Window) {
        LOG_ERROR("Failed to create glfw window");
        return false;
    }

    return true;
}

void Engine::init_vulkan()
{
#pragma region Instance

    vkb::InstanceBuilder builder;

    auto instance_builder = builder.set_app_name("Minecraft").require_api_version(1, 3, 0);

    if (enable_validation_layers) {
        instance_builder.request_validation_layers()
            .set_debug_callback(Logger::debug_callback);
    }

    const vkb::Instance vkb_inst = instance_builder.build().value();
    m_Instance = vkb_inst.instance;
    m_DebugMessenger = vkb_inst.debug_messenger;

    // Logger::log_available_extensions(vk::enumerateInstanceExtensionProperties().value);
    // Logger::log_available_layers(vk::enumerateInstanceLayerProperties().value);

#pragma endregion

#pragma region Device

    VkSurfaceKHR c_surface;
    glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &c_surface);
    m_Surface = c_surface;

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice vkb_physical_device = selector
                                                  .set_minimum_version(1, 3)
                                                  .set_surface(m_Surface)
                                                  .select()
                                                  .value();

    vkb::DeviceBuilder device_builder { vkb_physical_device };
    vkb::Device vkb_device = device_builder.build().value();

    m_PhysicalDevice = vkb_physical_device.physical_device;
    m_Device = vkb_device.device;

    Logger::log_device_properties(m_PhysicalDevice);

#pragma endregion

    m_PresentQueue = vkb_device.get_queue(vkb::QueueType::present).value();
    m_GraphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_GraphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void Engine::create_swapchain(const uint32_t width, const uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder(m_PhysicalDevice, m_Device, m_Surface);
    m_SwapChainImageFormat = vk::Format::eB8G8R8A8Unorm;

    const auto surface_format = vk::SurfaceFormatKHR { m_SwapChainImageFormat, vk::ColorSpaceKHR::eSrgbNonlinear };
    constexpr auto present_mode = static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo);
    constexpr auto image_usage_flags = static_cast<VkImageUsageFlags>(vk::ImageUsageFlagBits::eColorAttachment);
    constexpr auto composite_alpha_flags = static_cast<VkCompositeAlphaFlagBitsKHR>(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    vkb::Swapchain vkb_swapchain = swapchain_builder
                                       .set_desired_format(surface_format)
                                       .set_desired_present_mode(present_mode)
                                       .set_clipped()
                                       .set_desired_extent(width, height)
                                       .add_image_usage_flags(image_usage_flags)
                                       .set_composite_alpha_flags(composite_alpha_flags)
                                       .build()
                                       .value();

    m_SwapChainExtent = vkb_swapchain.extent;
    m_SwapChain = vkb_swapchain.swapchain;
    m_SwapChainImages = vkb_swapchain.get_images().value();
    m_SwapChainImageViews = vkb_swapchain.get_image_views().value();
}

void Engine::destroy_swapchain()
{
    m_Device.destroySwapchainKHR(m_SwapChain);

    for (const auto& image_view : m_SwapChainImageViews) {
        m_Device.destroyImageView(image_view);
    }
}

bool Engine::create_render_pass()
{
    const auto color_attachment = vk::AttachmentDescription({},
        m_SwapChainImageFormat,
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eDontCare,
        vk::AttachmentStoreOp::eDontCare,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR);

    // References to descriptions
    constexpr auto color_attachment_ref = vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal);

    // ReSharper disable once CppVariableCanBeMadeConstexpr
    const auto subpass = vk::SubpassDescription({},
        vk::PipelineBindPoint::eGraphics,
        0, {},
        1, &color_attachment_ref,
        nullptr,
        {},
        0, {});

    constexpr vk::SubpassDependency color_stage_dependency {
        vk::SubpassExternal,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentWrite
    };

    const vk::RenderPassCreateInfo renderpass_create_info { {},
        1, &color_attachment,
        1, &subpass,
        1, &color_stage_dependency
    };

    const auto [result, renderpass] = m_Device.createRenderPass(renderpass_create_info);
    VK_CHECK(result);

    m_RenderPass = renderpass;
    return true;
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

    constexpr auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo({},
        0, nullptr,
        0, nullptr);
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

    const auto [pipeline_layout_res, pipeline_layout] = m_Device.createPipelineLayout(pipeline_layout_info);
    VK_CHECK(pipeline_layout_res);
    m_PipelineLayout = pipeline_layout;

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
        m_RenderPass,
        0,
        VK_NULL_HANDLE,
        -1);

    const auto [pipeline_res, pipeline] = m_Device.createGraphicsPipeline(VK_NULL_HANDLE, pipeline_info);

    if (pipeline_res != vk::Result::eSuccess && pipeline_res != vk::Result::ePipelineCompileRequiredEXT) {
        LOG_ERROR("Vulkan error: {}", vk::to_string(pipeline_res));
        return false;
    }

    m_Pipeline = pipeline;
#pragma endregion

    m_Device.destroyShaderModule(vert_shader_module);
    m_Device.destroyShaderModule(frag_shader_module);
    return true;
}

bool Engine::create_framebuffers()
{
    m_SwapChainFramebuffers.resize(m_SwapChainImageViews.size());

    for (int i = 0; i < m_SwapChainImageViews.size(); i++) {
        const vk::ImageView attachments[] = {
            m_SwapChainImageViews[i]
        };

        const auto framebuffer_info = vk::FramebufferCreateInfo({},
            m_RenderPass,
            1,
            attachments,
            m_SwapChainExtent.width, m_SwapChainExtent.height,
            1);

        const auto [fb_res, framebuffer] = m_Device.createFramebuffer(framebuffer_info);
        VK_CHECK(fb_res);
        m_SwapChainFramebuffers[i] = framebuffer;
    }
    return true;
}

bool Engine::create_command_pool()
{
    const auto pool_info = vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        m_GraphicsQueueFamily);
    const auto [result, pool] = m_Device.createCommandPool(pool_info);

    VK_CHECK(result);

    m_FrameData.CommandPool = pool;
    return true;
}

bool Engine::allocate_command_buffer()
{
    const auto allocate_info = vk::CommandBufferAllocateInfo(
        m_FrameData.CommandPool, vk::CommandBufferLevel::ePrimary, 1);

    const auto [result, buffer] = m_Device.allocateCommandBuffers(allocate_info);
    VK_CHECK(result);

    m_FrameData.CommandBuffer = buffer[0]; // only 1 buffer created
    return true;
}

bool Engine::record_command_buffer(const vk::CommandBuffer& cmd, const uint32_t image_index) const
{
    vk::CommandBufferUsageFlagBits flags {};
    VK_CHECK(cmd.begin({ flags }));

    const vk::Rect2D scissor({ 0, 0 }, m_SwapChainExtent);

    vk::ClearValue clear_color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    const vk::RenderPassBeginInfo render_pass_info {
        m_RenderPass,
        m_SwapChainFramebuffers[image_index],
        scissor,
        1, &clear_color
    };

    cmd.beginRenderPass(render_pass_info, vk::SubpassContents::eInline);
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);

    // Scissor and viewport are bound here because are marked as dynamic on pipeline creation
    cmd.setScissor(0, 1, &scissor);

    const vk::Viewport viewport(0.0f, 0.0f,
        static_cast<float>(m_SwapChainExtent.width),
        static_cast<float>(m_SwapChainExtent.height),
        0.0f, 1.0f);
    cmd.setViewport(0, 1, &viewport);

    cmd.draw(3, 1, 0, 0);

    cmd.endRenderPass();

    VK_CHECK(cmd.end());
    return true;
}

bool Engine::create_sync_objects()
{
    const auto [s1_res, s1] = m_Device.createSemaphore({});
    VK_CHECK(s1_res);
    m_FrameData.SwapChainSemaphore = s1;

    const auto [s2_res, s2] = m_Device.createSemaphore({});
    VK_CHECK(s2_res);
    m_FrameData.RenderSemaphore = s2;

    constexpr vk::FenceCreateInfo fence_info {
        vk::FenceCreateFlagBits::eSignaled
    };
    const auto [f_res, f] = m_Device.createFence(fence_info);
    VK_CHECK(f_res);
    m_FrameData.RenderFence = f;

    return true;
}

bool Engine::draw_frame() const
{
    VK_CHECK(m_Device.waitForFences(1, &m_FrameData.RenderFence, vk::True, UINT64_MAX));

    VK_CHECK(m_Device.resetFences(1, &m_FrameData.RenderFence));

    uint32_t image_index;
    VK_CHECK(m_Device.acquireNextImageKHR(
        m_SwapChain, UINT64_MAX, m_FrameData.SwapChainSemaphore, VK_NULL_HANDLE, &image_index));

    const vk::CommandBuffer& cmd = m_FrameData.CommandBuffer;
    VK_CHECK(cmd.reset());
    if (!record_command_buffer(cmd, image_index)) {
        LOG_ERROR("Failed to record on the command buffer");
        return false;
    }

    vk::Semaphore wait_semaphores[] = { m_FrameData.SwapChainSemaphore };
    vk::PipelineStageFlags wait_stages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
    vk::Semaphore signal_semaphores[] = { m_FrameData.RenderSemaphore };

    const vk::SubmitInfo submit_info {
        1, wait_semaphores, wait_stages,
        1, &cmd,
        1, signal_semaphores
    };

    VK_CHECK(m_GraphicsQueue.submit(1, &submit_info, m_FrameData.RenderFence));

    vk::SwapchainKHR swapchains[] = { m_SwapChain };

    const vk::PresentInfoKHR present_info {
        1, signal_semaphores,
        1, swapchains, &image_index,
        nullptr
    };

    VK_CHECK(m_PresentQueue.presentKHR(present_info));

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
    }
    VK_CHECK(m_Device.waitIdle());
    LOG("Engine stopped");
    return true;
}

Engine::~Engine()
{
    LOG("GfxManager destructor");

    // Sync Objects
    if (m_FrameData.SwapChainSemaphore) {
        m_Device.destroySemaphore(m_FrameData.SwapChainSemaphore);
    }

    if (m_FrameData.RenderSemaphore) {
        m_Device.destroySemaphore(m_FrameData.RenderSemaphore);
    }

    if (m_FrameData.RenderFence) {
        m_Device.destroyFence(m_FrameData.RenderFence);
    }

    if (m_FrameData.CommandPool) {
        m_Device.destroyCommandPool(m_FrameData.CommandPool);
    }

    // Graphics pipeline
    for (const auto& framebuffer : m_SwapChainFramebuffers) {
        m_Device.destroyFramebuffer(framebuffer);
    }

    if (m_Pipeline) {
        m_Device.destroyPipeline(m_Pipeline);
    }

    if (m_PipelineLayout) {
        m_Device.destroyPipelineLayout(m_PipelineLayout);
    }

    if (m_RenderPass) {
        m_Device.destroyRenderPass(m_RenderPass);
    }

    // Core
    if (m_SwapChain) {
        destroy_swapchain();
    }

    if (m_Surface) {
        m_Instance.destroySurfaceKHR(m_Surface);
    }

    if (m_Device) {
        m_Device.destroy();
    }

    if (m_Instance) {
        if (enable_validation_layers) {
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
        }
        m_Instance.destroy();
    }

    LOG("Window destructor");
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}