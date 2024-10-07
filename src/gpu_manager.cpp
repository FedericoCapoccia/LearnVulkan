#include "gpu_manager.hpp"

#define VMA_VULKAN_VERSION 1003000
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Minecraft::VkEngine {

ResourcesBundle GpuManager::init(const GpuManagerSpec& spec)
{
    assert(!m_Initialized);

    glfwGetFramebufferSize(spec.Window,
        reinterpret_cast<int32_t*>(&m_WindowExtent.width),
        reinterpret_cast<int32_t*>(&m_WindowExtent.height));

#pragma region InstanceCreation

    vkb::InstanceBuilder builder;
    builder
        .set_app_name(spec.AppName)
        .require_api_version(1, 3, 0);

    if (spec.EnableValidation) {
        builder.request_validation_layers();
        if (spec.DebugCallback.has_value()) {
            builder.set_debug_callback(spec.DebugCallback.value());
        } else {
            builder.use_default_debug_messenger();
        }
    }

    const vkb::Instance vkb_instance = builder.build().value();

    m_Instance = vkb_instance.instance;
    m_DeletionQueue.push_function("Instance", [&] {
        m_Instance.destroy();
    });

    if (spec.EnableValidation) {
        m_DebugMessenger = vkb_instance.debug_messenger;
        m_DeletionQueue.push_function("Debug Messenger", [&] {
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
        });
    }

#pragma endregion

#pragma region SurfaceSetup

    VkSurfaceKHR c_surface;
    glfwCreateWindowSurface(m_Instance, spec.Window, nullptr, &c_surface);
    m_Surface = c_surface;
    m_DeletionQueue.push_function("Surface", [&] {
        m_Instance.destroySurfaceKHR(m_Surface);
    });

#pragma endregion

#pragma region DeviceSetup

    vk::PhysicalDeviceVulkan13Features features13;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    vk::PhysicalDeviceVulkan12Features features12;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector { vkb_instance };
    selector
        .set_surface(m_Surface)
        .set_minimum_version(1, 3)
        .require_present()
        .set_required_features_13(features13)
        .set_required_features_12(features12);

    const vkb::PhysicalDevice vkb_physical_device = selector.select().value();
    const vkb::DeviceBuilder device_builder { vkb_physical_device };

    const vkb::Device vkb_device = device_builder.build().value();

    m_PhysicalDevice = vkb_physical_device.physical_device;
    m_Device = vkb_device.device;

    m_DeletionQueue.push_function("Device", [&] {
        m_Device.destroy();
    });

#pragma endregion

#pragma region Allocator

    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_3;
    allocator_create_info.physicalDevice = m_PhysicalDevice;
    allocator_create_info.device = m_Device;
    allocator_create_info.instance = m_Instance;
    allocator_create_info.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocator_create_info, &m_Allocator);
    m_DeletionQueue.push_function("VMA", [&] {
        vmaDestroyAllocator(m_Allocator);
    });

#pragma endregion

    init_swapchain();

    m_Initialized = true;

    const QueueBundle queue_bundle = {
        vkb_device.get_queue(vkb::QueueType::graphics).value(),
        vkb_device.get_queue_index(vkb::QueueType::graphics).value()
    };

    return ResourcesBundle {
        queue_bundle,
        m_SwapchainBundle,
        m_Device
    };
}

void GpuManager::destroy_swapchain()
{
    m_Device.destroySwapchainKHR(m_SwapchainBundle.Handle);

    for (const auto& image_view : m_SwapchainBundle.ImageViews) {
        m_Device.destroyImageView(image_view);
    }
}

void GpuManager::create_swapchain(const uint32_t width, const uint32_t height)
{
    vkb::SwapchainBuilder builder(m_PhysicalDevice, m_Device, m_Surface);

    m_SwapchainBundle.ImageFormat = vk::Format::eB8G8R8A8Unorm;

    builder
        .set_desired_format(vk::SurfaceFormatKHR { m_SwapchainBundle.ImageFormat, vk::ColorSpaceKHR::eSrgbNonlinear })
        .set_desired_present_mode(static_cast<VkPresentModeKHR>(vk::PresentModeKHR::eFifo))
        .set_desired_extent(width, height)
        .add_image_usage_flags(static_cast<VkImageUsageFlags>(vk::ImageUsageFlagBits::eTransferDst))
        .set_composite_alpha_flags(static_cast<VkCompositeAlphaFlagBitsKHR>(vk::CompositeAlphaFlagBitsKHR::eOpaque));

    vkb::Swapchain vkb_swapchain = builder.build().value();

    m_SwapchainBundle.Handle = vkb_swapchain.swapchain;
    m_SwapchainBundle.Images = vkb_swapchain.get_images().value();
    m_SwapchainBundle.ImageViews = vkb_swapchain.get_image_views().value();
    m_SwapchainBundle.Extent = vkb_swapchain.extent;
}

void GpuManager::init_swapchain()
{
    create_swapchain(m_WindowExtent.width, m_WindowExtent.height);

    // TODO chapter 2 - Image allocations & https://github.com/vblanco20-1/vulkan-guide/blob/all-chapters-2/chapter-2/vk_engine.cpp#L33
}

}