#include "gpu_manager.hpp"
#include "helper.hpp"
#include "logger.hpp"

#define VMA_VULKAN_VERSION 1003000
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Minecraft::VkEngine {

std::expected<vk::Image, vk::Result> GpuManager::get_next_swapchain_image(const vk::Semaphore swapchain_semaphore, const uint64_t timeout)
{
    if (const vk::Result res = m_Device.acquireNextImageKHR(m_SwapchainBundle.Handle, timeout, swapchain_semaphore,
            VK_NULL_HANDLE, &m_CurrentSwapchainImageIndex);
        res == vk::Result::eErrorOutOfDateKHR) {
        // TODO rebuild swapchain
    } else if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) {
        m_CurrentSwapchainImageIndex = -1;
        return std::unexpected(res);
    }

    m_CurrentSwapchainImage = m_SwapchainBundle.Images[m_CurrentSwapchainImageIndex];
    return m_CurrentSwapchainImage;
}

vk::Result GpuManager::submit_to_queue(const vk::SubmitInfo2& submit_info2, const vk::Fence render_fence) const
{
    return m_GraphicsQueue.Queue.submit2(1, &submit_info2, render_fence);
}

vk::Result GpuManager::present(const uint32_t semaphores_count, vk::Semaphore* semaphores)
{
    const vk::PresentInfoKHR present_info {
        semaphores_count,
        semaphores,
        1,
        &m_SwapchainBundle.Handle,
        &m_CurrentSwapchainImageIndex,
    };

    const vk::Result res = m_GraphicsQueue.Queue.presentKHR(present_info);
    if (res == vk::Result::eErrorOutOfDateKHR || res == vk::Result::eSuboptimalKHR) {
        // TODO rebuild swapchain
        return vk::Result::eSuccess;
    }

    return res;
}

std::expected<vk::CommandPool, vk::Result> GpuManager::create_command_pool(const vk::CommandPoolCreateFlags flags)
{
    const vk::CommandPoolCreateInfo info { flags, m_GraphicsQueue.FamilyIndex };
    const auto [res, pool] = m_Device.createCommandPool(info);

    if (res != vk::Result::eSuccess) {
        return std::unexpected(res);
    }

    m_CommandPools.push_back(pool);
    return pool;
}

std::expected<vk::CommandBuffer, vk::Result> GpuManager::allocate_command_buffer(const vk::CommandPool pool, const vk::CommandBufferLevel level) const
{
    const vk::CommandBufferAllocateInfo info { pool, level, 1 };
    const auto [res, buffer] = m_Device.allocateCommandBuffers(info);

    if (res != vk::Result::eSuccess) {
        return std::unexpected(res);
    }

    return buffer[0];
}

std::expected<vk::Semaphore, vk::Result> GpuManager::create_semaphore(const vk::SemaphoreCreateFlags flags)
{
    const vk::SemaphoreCreateInfo info { flags };
    const auto [res, semaphore] = m_Device.createSemaphore(info);

    if (res != vk::Result::eSuccess) {
        return std::unexpected(res);
    }

    m_Semaphores.push_back(semaphore);
    return semaphore;
}

std::expected<vk::Fence, vk::Result> GpuManager::create_fence(const vk::FenceCreateFlags flags)
{
    const vk::FenceCreateInfo info { flags };
    const auto [res, fence] = m_Device.createFence(info);

    if (res != vk::Result::eSuccess) {
        return std::unexpected(res);
    }

    m_Fences.push_back(fence);
    return fence;
}

vk::Result GpuManager::wait_fence(const vk::Fence fence, const uint64_t timeout) const
{
    return m_Device.waitForFences(1, &fence, vk::True, timeout);
}

vk::Result GpuManager::reset_fence(const vk::Fence fence) const
{
    return m_Device.resetFences(1, &fence);
}

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

    m_GraphicsQueue = {
        vkb_device.get_queue(vkb::QueueType::graphics).value(),
        vkb_device.get_queue_index(vkb::QueueType::graphics).value()
    };

    const DrawImageBundle image_bundle = {
        .Image = m_DrawImage.Image,
        .ImageView = m_DrawImage.ImageView,
        .Extent = m_DrawImage.Extent,
        .Format = m_DrawImage.Format
    };

    // TODO remove everything
    return ResourcesBundle {
        m_Device,
        image_bundle
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
    const vk::Extent3D draw_image_extent {
        m_WindowExtent.width,
        m_WindowExtent.height,
        1
    };

    m_DrawImage.Format = vk::Format::eR16G16B16A16Sfloat;
    m_DrawImage.Extent = draw_image_extent;

    vk::ImageUsageFlags draw_image_usage {};
    draw_image_usage |= vk::ImageUsageFlagBits::eTransferSrc;
    draw_image_usage |= vk::ImageUsageFlagBits::eTransferDst;
    draw_image_usage |= vk::ImageUsageFlagBits::eStorage;
    draw_image_usage |= vk::ImageUsageFlagBits::eColorAttachment;

    // Image Allocation
    vk::ImageCreateInfo rimg_info = VkInit::image_create_info(m_DrawImage.Format, draw_image_usage, draw_image_extent);
    VmaAllocationCreateInfo rimg_alloc_info = {};
    rimg_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_alloc_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlagBits::eDeviceLocal);

    VkImage image_c;
    vmaCreateImage(m_Allocator, reinterpret_cast<VkImageCreateInfo*>(&rimg_info), &rimg_alloc_info,
        &image_c, &m_DrawImage.Allocation, nullptr);
    m_DrawImage.Image = image_c;

    // ImageView Allocation
    const vk::ImageViewCreateInfo rview_info = VkInit::imageview_create_info(m_DrawImage.Format, m_DrawImage.Image, vk::ImageAspectFlagBits::eColor);
    if (const vk::Result res = m_Device.createImageView(&rview_info, nullptr, &m_DrawImage.ImageView); res != vk::Result::eSuccess) {
        LOG_ERROR("Failed to allocate Image View");
    }

    m_DeletionQueue.push_function("swapchain init", [&] {
        m_Device.destroyImageView(m_DrawImage.ImageView);
        vmaDestroyImage(m_Allocator, m_DrawImage.Image, m_DrawImage.Allocation);
    });
}

}