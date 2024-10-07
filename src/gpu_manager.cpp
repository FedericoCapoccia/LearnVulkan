#include "gpu_manager.hpp"

#define VMA_VULKAN_VERSION 1003000
#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Minecraft::VkEngine {

QueueBundle GpuManager::get_queue(const vkb::QueueType queue_type, const bool dedicated) const
{
    vk::Queue queue;
    uint32_t queue_index;
    if (dedicated) {
        queue = m_VkbDevice.get_dedicated_queue(queue_type).value();
        queue_index = m_VkbDevice.get_dedicated_queue_index(queue_type).value();
    } else {
        queue = m_VkbDevice.get_queue(queue_type).value();
        queue_index = m_VkbDevice.get_queue_index(queue_type).value();
    }
    return {
        queue,
        queue_index
    };
}

std::expected<vk::Buffer, const char*> GpuManager::create_vertex_buffer(const std::vector<Vertex>& vertices)
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
        return std::unexpected("Vertex buffer allocation failed");
    }

    m_VertexBuffer = {
        .Handle = buffer,
        .Allocation = allocation
    };

    void* data;
    vmaMapMemory(m_Allocator, m_VertexBuffer.Allocation, &data);

    memcpy(data, vertices.data(), vertices.size() * sizeof(Vertex));

    vmaUnmapMemory(m_Allocator, m_VertexBuffer.Allocation);

    m_DeletionQueue.push_function("Vertex Buffer", [&] {
        vmaDestroyBuffer(m_Allocator, m_VertexBuffer.Handle, m_VertexBuffer.Allocation);
    });

    return m_VertexBuffer.Handle;
}

const SwapchainBundle& GpuManager::init(const GpuManagerSpec& spec)
{
    assert(!m_Initialized);

    init_instance(spec.InstanceSpec_);
    create_surface(spec.SwapchainSpec_.Window);
    init_device(spec.DeviceSpec_);

    init_allocator();

    create_swapchain(spec.SwapchainSpec_);
    create_render_pass();
    create_framebuffers();

    m_Initialized = true;
    return m_SwapchainBundle;
}

const SwapchainBundle& GpuManager::recreate_swapchain(const SwapchainSpec& spec)
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(spec.Window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(spec.Window, &width, &height);
        glfwWaitEvents();
    }

    create_swapchain(spec);
    create_framebuffers();

    return m_SwapchainBundle;
}

void GpuManager::init_instance(const InstanceSpec& spec)
{
    vkb::InstanceBuilder builder;
    builder
        .set_app_name(spec.AppName)
        .require_api_version(1, 3, 0)
        .enable_extensions(spec.Extensions);

    if (spec.EnableValidationLayers) {
        builder.request_validation_layers();
        if (spec.DebugCallback.has_value()) {
            builder.set_debug_callback(spec.DebugCallback.value());
        } else {
            builder.use_default_debug_messenger();
        }
    }

    m_VkbInstance = builder.build().value();

    m_Instance = m_VkbInstance.instance;
    m_DeletionQueue.push_function("Instance", [&] {
        m_Instance.destroy();
    });

    if (spec.EnableValidationLayers) {
        m_DebugMessenger = m_VkbInstance.debug_messenger;
        m_DeletionQueue.push_function("Debug Messenger", [&] {
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
        });
    }
}

// TODO make it glfw independent with Window abstraction
void GpuManager::create_surface(GLFWwindow* window)
{
    VkSurfaceKHR c_surface;
    glfwCreateWindowSurface(m_Instance, window, nullptr, &c_surface);
    m_Surface = c_surface;
    m_DeletionQueue.push_function("Surface", [&] {
        m_Instance.destroySurfaceKHR(m_Surface);
    });
}

void GpuManager::init_device(const DeviceSpec& spec)
{
    vk::PhysicalDeviceVulkan13Features features13;
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    vk::PhysicalDeviceVulkan12Features features12;
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector { m_VkbInstance };
    selector
        .set_surface(m_Surface)
        .set_minimum_version(1, 3)
        .require_present()
        .set_required_features_13(features13)
        .set_required_features_12(features12);

    if (spec.RequireDedicatedComputeQueue) {
        selector.require_dedicated_compute_queue();
    }

    if (spec.RequireDedicatedTransferQueue) {
        selector.require_dedicated_transfer_queue();
    }

    // TODO fix this so it doesn't crash if dedicated queue are requested and not valid smh...
    const vkb::PhysicalDevice vkb_physical_device = selector.select().value();

    const vkb::DeviceBuilder device_builder { vkb_physical_device };

    m_VkbDevice = device_builder.build().value();

    m_PhysicalDevice = vkb_physical_device.physical_device;

    m_Device = m_VkbDevice.device;
    m_DeletionQueue.push_function("Device", [&] {
        m_Device.destroy();
    });
}

void GpuManager::init_allocator()
{
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
    m_DeletionQueue.push_function("VMA", [&] {
        vmaDestroyAllocator(m_Allocator);
    });
}

void GpuManager::create_swapchain(const SwapchainSpec& spec)
{
    m_SwapchainDeletionQueue.flush();

    int width = 0, height = 0;
    glfwGetFramebufferSize(spec.Window, &width, &height);

    vkb::SwapchainBuilder builder(m_PhysicalDevice, m_Device, m_Surface);
    builder
        .set_desired_format(spec.Format)
        .set_desired_present_mode(static_cast<VkPresentModeKHR>(spec.PresentMode))
        .set_clipped()
        .set_desired_extent(width, height)
        .add_image_usage_flags(static_cast<VkImageUsageFlags>(spec.ImageUsageFlagBits))
        .set_composite_alpha_flags(static_cast<VkCompositeAlphaFlagBitsKHR>(spec.CompositeAlphaFlagBits));

    vkb::Swapchain vkb_swapchain = builder.build().value();

    m_SwapchainBundle.Swapchain = vkb_swapchain.swapchain;
    m_SwapchainBundle.ImageFormat = spec.Format.format;
    m_SwapchainBundle.Images = vkb_swapchain.get_images().value();
    m_SwapchainBundle.ImageViews = vkb_swapchain.get_image_views().value();
    m_SwapchainBundle.Extent = vkb_swapchain.extent;

    m_SwapchainDeletionQueue.push_function("Swapchain callback", [&] {
        cleanup_swapchain();
    });
}

void GpuManager::create_render_pass()
{
    const auto color_attachment = vk::AttachmentDescription({},
        m_SwapchainBundle.ImageFormat,
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

    vk::Result _ = m_Device.createRenderPass(&renderpass_create_info, nullptr, &m_RenderPass);
    (void)_;

    m_DeletionQueue.push_function("Render pass", [&] {
        m_Device.destroyRenderPass(m_RenderPass);
    });
}

void GpuManager::create_framebuffers()
{
    const size_t image_views_size = m_SwapchainBundle.ImageViews.size();

    m_SwapchainBundle.Framebuffers.resize(image_views_size);

    for (int i = 0; i < image_views_size; i++) {
        const vk::ImageView attachments[] = {
            m_SwapchainBundle.ImageViews[i]
        };

        const auto framebuffer_info = vk::FramebufferCreateInfo({},
            m_RenderPass,
            1,
            attachments,
            m_SwapchainBundle.Extent.width, m_SwapchainBundle.Extent.height,
            1);

        vk::Result _ = m_Device.createFramebuffer(&framebuffer_info, nullptr, &m_SwapchainBundle.Framebuffers[i]);
        (void)_;
    }
}

void GpuManager::cleanup_swapchain()
{
    const vk::Result _ = m_Device.waitIdle();
    (void)_;

    for (const auto& framebuffer : m_SwapchainBundle.Framebuffers) {
        m_Device.destroyFramebuffer(framebuffer);
    }

    for (const auto& image_view : m_SwapchainBundle.ImageViews) {
        m_Device.destroyImageView(image_view);
    }

    (void)_;

    if (m_SwapchainBundle.Swapchain) {
        m_Device.destroySwapchainKHR(m_SwapchainBundle.Swapchain);
    }

    m_SwapchainBundle = {};
}

}