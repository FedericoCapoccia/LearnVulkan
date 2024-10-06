#include "engine.hpp"
#include "logger.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Minecraft {

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
    create_swapchain();

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

    if (!create_vertex_buffer()) {
        LOG_ERROR("Failed to create vertex buffer");
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

static void framebuffer_resize_callback(GLFWwindow* window, int width, int height)
{
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
#pragma region Instance

    vkb::InstanceBuilder builder;

    auto instance_builder = builder.set_app_name("Minecraft")
                                .require_api_version(1, 3, 0)
                                .enable_layer("VK_LAYER_MANGOHUD_overlay_x86_64");

    if (enable_validation_layers) {
        instance_builder.request_validation_layers()
            .set_debug_callback(Logger::debug_callback);
    }

    const vkb::Instance vkb_inst = instance_builder.build().value();
    m_Instance = vkb_inst.instance;
    m_DebugMessenger = vkb_inst.debug_messenger;

    // Logger::log_available_extensions(vk::enumerateInstanceExtensionProperties().value);
    Logger::log_available_layers(vk::enumerateInstanceLayerProperties().value);

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

#define VMA_VULKAN_VERSION 1003000

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_create_info.physicalDevice = m_PhysicalDevice;
    allocator_create_info.device = m_Device;
    allocator_create_info.instance = m_Instance;
    allocator_create_info.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocator_create_info, &m_Allocator);
}

void Engine::create_swapchain()
{
    vkb::SwapchainBuilder swapchain_builder(m_PhysicalDevice, m_Device, m_Surface);
    m_SwapChainImageFormat = vk::Format::eB8G8R8A8Unorm;

    const auto surface_format = vk::SurfaceFormatKHR { m_SwapChainImageFormat, vk::ColorSpaceKHR::eSrgbNonlinear };
    constexpr auto present_mode = static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo);
    constexpr auto image_usage_flags = static_cast<VkImageUsageFlags>(vk::ImageUsageFlagBits::eColorAttachment);
    constexpr auto composite_alpha_flags = static_cast<VkCompositeAlphaFlagBitsKHR>(vk::CompositeAlphaFlagBitsKHR::eOpaque);

    int width, height;
    glfwGetFramebufferSize(m_Window, &width, &height);
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

void Engine::cleanup_swapchain()
{
    for (const auto& framebuffer : m_SwapChainFramebuffers) {
        m_Device.destroyFramebuffer(framebuffer);
    }

    for (const auto& image_view : m_SwapChainImageViews) {
        m_Device.destroyImageView(image_view);
    }

    m_Device.destroySwapchainKHR(m_SwapChain);
}

bool Engine::recreate_swapchain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_Window, &width, &height);
        glfwWaitEvents();
    }

    VK_CHECK(m_Device.waitIdle());

    cleanup_swapchain();

    create_swapchain();

    if (!create_framebuffers()) {
        LOG_ERROR("Failed to create framebuffers");
        return false;
    }

    return true;
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
        1, &color_stage_dependency };

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

bool Engine::init_commands()
{
    constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    const auto pool_info = vk::CommandPoolCreateInfo(flags, m_GraphicsQueueFamily);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VK_CHECK(m_Device.createCommandPool(&pool_info, nullptr, &m_Frames[i].CommandPool));

        const auto allocate_info = vk::CommandBufferAllocateInfo(
            m_Frames[i].CommandPool, vk::CommandBufferLevel::ePrimary, 1);

        VK_CHECK(m_Device.allocateCommandBuffers(&allocate_info, &m_Frames[i].CommandBuffer));
    }

    return true;
}

bool Engine::create_vertex_buffer()
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = sizeof(vertices[0]) * vertices.size();
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info {};
    alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    VkBuffer buffer;
    VmaAllocation allocation;

    if (const VkResult result = vmaCreateBuffer(m_Allocator, &buffer_info, &alloc_info, &buffer, &allocation, nullptr); result != VK_SUCCESS) {
        LOG_ERROR("Vertex buffer allocation failed");
        return false;
    }

    m_VertexBuffer = {
        .Handle = buffer,
        .Allocation = allocation
    };

    void* data;
    vmaMapMemory(m_Allocator, m_VertexBuffer.Allocation, &data);

    memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));

    vmaUnmapMemory(m_Allocator, m_VertexBuffer.Allocation);

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

    const vk::Buffer vertexBuffers[] = { m_VertexBuffer.Handle };
    // ReSharper disable once CppRedundantZeroInitializerInAggregateInitialization
    constexpr vk::DeviceSize offsets[] = { 0 };

    cmd.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    // Scissor and viewport are bound here because are marked as dynamic on pipeline creation
    cmd.setScissor(0, 1, &scissor);

    const vk::Viewport viewport(0.0f, 0.0f,
        static_cast<float>(m_SwapChainExtent.width),
        static_cast<float>(m_SwapChainExtent.height),
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
        m_SwapChain, UINT64_MAX, get_current_frame().SwapChainSemaphore, VK_NULL_HANDLE, &image_index);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        if (!recreate_swapchain()) {
            LOG_ERROR("failed to recreate swapchain");
            return false;
        }
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

    VK_CHECK(m_GraphicsQueue.submit(1, &submit_info, get_current_frame().RenderFence));

    vk::SwapchainKHR swapchains[] = { m_SwapChain };

    const vk::PresentInfoKHR present_info {
        1, signal_semaphores,
        1, swapchains, &image_index,
        nullptr
    };

    result = m_PresentQueue.presentKHR(present_info);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || m_FramebufferResized) {
        m_FramebufferResized = false;
        if (!recreate_swapchain()) {
            LOG_ERROR("failed to recreate swapchain");
            return false;
        }
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
    if (m_SwapChain) {
        cleanup_swapchain();
    }

    vmaDestroyBuffer(m_Allocator, m_VertexBuffer.Handle, m_VertexBuffer.Allocation);

    // Graphics pipeline
    if (m_Pipeline) {
        m_Device.destroyPipeline(m_Pipeline);
    }
    if (m_PipelineLayout) {
        m_Device.destroyPipelineLayout(m_PipelineLayout);
    }
    if (m_RenderPass) {
        m_Device.destroyRenderPass(m_RenderPass);
    }

    if (m_Frames[0].CommandPool) {
        // Sync Objects
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            m_Device.destroyCommandPool(m_Frames[i].CommandPool);

            m_Device.destroyFence(m_Frames[i].RenderFence);
            m_Device.destroySemaphore(m_Frames[i].RenderSemaphore);
            m_Device.destroySemaphore(m_Frames[i].SwapChainSemaphore);
        }
    }

    vmaDestroyAllocator(m_Allocator);

    if (m_Device) {
        m_Device.destroy();
    }

    if (m_Instance) {
        if (m_Surface) {
            m_Instance.destroySurfaceKHR(m_Surface);
        }
        if (enable_validation_layers) {
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
        }
        m_Instance.destroy();
    }

    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}