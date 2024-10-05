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


bool Engine::run()
{
    m_Running = true;
    LOG("Engine started");
    while (m_Running) {
        glfwPollEvents();
        m_Running = !glfwWindowShouldClose(m_Window);
        m_Running = false; // TODO remove me
    }
    LOG("Engine stopped");
    return true;
}

Engine::~Engine()
{
    LOG("GfxManager destructor");

    vkDeviceWaitIdle(m_Device);

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