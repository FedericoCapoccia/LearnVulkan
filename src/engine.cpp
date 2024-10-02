#include "engine.hpp"
#include "images.hpp"
#include "vk_init.hpp"
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

    if (!init_commands())
        return false;

    if (!init_sync_structures())
        return false;

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

    Logger::log_available_extensions(vk::enumerateInstanceExtensionProperties().value);
    Logger::log_available_layers(vk::enumerateInstanceLayerProperties().value);

#pragma endregion

#pragma region Device

    VkSurfaceKHR c_surface;
    glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &c_surface);
    m_Surface = c_surface;

    vk::PhysicalDeviceVulkan13Features features;
    features.dynamicRendering = true;
    features.synchronization2 = true;

    vk::PhysicalDeviceVulkan12Features features12;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector { vkb_inst };
    vkb::PhysicalDevice vkb_physical_device = selector
                                                  .set_minimum_version(1, 3)
                                                  .set_required_features_13(features)
                                                  .set_required_features_12(features12)
                                                  .set_surface(m_Surface)
                                                  .select()
                                                  .value();

    vkb::DeviceBuilder device_builder { vkb_physical_device };
    vkb::Device vkb_device = device_builder.build().value();

    m_PhysicalDevice = vkb_physical_device.physical_device;
    m_Device = vkb_device.device;

    Logger::log_device_properties(m_PhysicalDevice);

#pragma endregion

    m_GraphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    m_GraphicsQueueFamily = vkb_device.get_queue_index(vkb::QueueType::graphics).value();
}

void Engine::create_swapchain(const uint32_t width, const uint32_t height)
{
    vkb::SwapchainBuilder swapchain_builder(m_PhysicalDevice, m_Device, m_Surface);
    m_SwapchainImageFormat = vk::Format::eB8G8R8A8Unorm;

    const auto surface_format = vk::SurfaceFormatKHR { m_SwapchainImageFormat, vk::ColorSpaceKHR::eSrgbNonlinear };
    constexpr auto present_mode = static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo);
    constexpr auto image_usage_flags = static_cast<VkImageUsageFlags>(vk::ImageUsageFlagBits::eTransferDst);

    vkb::Swapchain vkb_swapchain = swapchain_builder
                                       .set_desired_format(surface_format)
                                       .set_desired_present_mode(present_mode)
                                       .set_desired_extent(width, height)
                                       .add_image_usage_flags(image_usage_flags)
                                       .build()
                                       .value();

    m_SwapchainExtent = vkb_swapchain.extent;
    m_Swapchain = vkb_swapchain.swapchain;
    m_SwapchainImages = vkb_swapchain.get_images().value();
    m_SwapchainImageViews = vkb_swapchain.get_image_views().value();
}

void Engine::destroy_swapchain()
{
    m_Device.destroySwapchainKHR(m_Swapchain);

    for (const auto& image_view : m_SwapchainImageViews) {
        m_Device.destroyImageView(image_view);
    }
}

bool Engine::init_commands()
{
    constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    const auto command_pool_create_info = vk::CommandPoolCreateInfo(flags, m_GraphicsQueueFamily);

    for (int i = 0; i < FRAME_OVERLAP; i++) {

        VK_CHECK(m_Device.createCommandPool(&command_pool_create_info, nullptr, &m_Frames[i].CommandPool));

        const auto command_buffer_allocate_info = vk::CommandBufferAllocateInfo(
            m_Frames[i].CommandPool, vk::CommandBufferLevel::ePrimary, 1);

        VK_CHECK(m_Device.allocateCommandBuffers(&command_buffer_allocate_info, &m_Frames[i].CommandBuffer));
    }

    return true;
}

bool Engine::init_sync_structures()
{
    constexpr auto fence_create_info = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);
    constexpr auto semaphore_create_info = vk::SemaphoreCreateInfo();

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(m_Device.createFence(&fence_create_info, nullptr, &m_Frames[i].RenderFence));
        VK_CHECK(m_Device.createSemaphore(&semaphore_create_info, nullptr, &m_Frames[i].SwapchainSemaphore));
        VK_CHECK(m_Device.createSemaphore(&semaphore_create_info, nullptr, &m_Frames[i].RenderSemaphore));
    }

    return true;
}

bool Engine::draw()
{
    VK_CHECK(m_Device.waitForFences(1, &get_current_frame().RenderFence, true, 1000000000));
    VK_CHECK(m_Device.resetFences(1, &get_current_frame().RenderFence));

    uint32_t swapchain_image_index;
    VK_CHECK(m_Device.acquireNextImageKHR(
        m_Swapchain, 1000000000,
        get_current_frame().SwapchainSemaphore, nullptr, &swapchain_image_index));

    const vk::CommandBuffer cmd = get_current_frame().CommandBuffer;
    VK_CHECK(cmd.reset());
    constexpr auto cmd_begin_info = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    VK_CHECK(cmd.begin(&cmd_begin_info));

    /* TODO find better alternatives to eGeneral as layout because I won't be using compute shaders
     * https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap12.html#resources-image-layouts
     */
    VkUtil::transition_image(cmd, m_SwapchainImages[swapchain_image_index],
        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    const float flash = std::abs(std::sin(static_cast<float>(m_FrameNumber) / 120.f));
    // constexpr vk::ClearColorValue clear_value = { 1.0f, 0.0f, 1.0f, 1.0f };
    const vk::ClearColorValue clear_value = { 0.0f, 0.0f, flash, 1.0f };

    const vk::ImageSubresourceRange clear_range = VkInit::image_subresource_range(vk::ImageAspectFlagBits::eColor);

    cmd.clearColorImage(m_SwapchainImages[swapchain_image_index],
        vk::ImageLayout::eGeneral, &clear_value, 1, &clear_range);

    VkUtil::transition_image(cmd, m_SwapchainImages[swapchain_image_index],
        vk::ImageLayout::eGeneral, vk::ImageLayout::ePresentSrcKHR);

    VK_CHECK(cmd.end()); // TODO check if more than vk::Result::eSuccess is success

    auto cmd_info = VkInit::command_buffer_submit_info(cmd);
    auto wait_info = VkInit::semaphore_submit_info(vk::PipelineStageFlagBits2KHR::eColorAttachmentOutput,
        get_current_frame().SwapchainSemaphore);
    auto signal_info = VkInit::semaphore_submit_info(vk::PipelineStageFlagBits2::eAllGraphics,
        get_current_frame().RenderSemaphore);

    const auto submit = VkInit::submit_info(&cmd_info, &signal_info, &wait_info);

    VK_CHECK(m_GraphicsQueue.submit2(1, &submit, get_current_frame().RenderFence));

    const auto present_info = vk::PresentInfoKHR(
        1, &get_current_frame().RenderSemaphore,
        1, &m_Swapchain, &swapchain_image_index);

    VK_CHECK(m_GraphicsQueue.presentKHR(&present_info));

    m_FrameNumber++;
    return true;
}

bool Engine::run()
{
    m_Running = true;
    LOG("Engine started");
    while (m_Running) {
        draw();

        glfwPollEvents();
        m_Running = !glfwWindowShouldClose(m_Window);
        //m_Running = false; // TODO remove me
    }
    LOG("Engine stopped");
    return true;
}

Engine::~Engine()
{
    LOG("GfxManager destructor");

    vkDeviceWaitIdle(m_Device);
    for (int i = 0; i < FRAME_OVERLAP; i++) {
        m_Device.destroyCommandPool(m_Frames[i].CommandPool);

        m_Device.destroyFence(m_Frames[i].RenderFence);
        m_Device.destroySemaphore(m_Frames[i].RenderSemaphore);
        m_Device.destroySemaphore(m_Frames[i].SwapchainSemaphore);
    }

    destroy_swapchain();

    m_Instance.destroySurfaceKHR(m_Surface);
    m_Device.destroy();

    if (enable_validation_layers) {
        vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    }
    m_Instance.destroy();

    LOG("Window destructor");
    glfwDestroyWindow(m_Window);
    glfwTerminate();
}

}