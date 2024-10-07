#include "gpu_manager.hpp"
#include "logger.hpp"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Minecraft::VulkanEngine {

void GpuManager::init(GLFWwindow* window)
{
    #pragma region Instance

    vkb::InstanceBuilder builder;

    auto instance_builder = builder.set_app_name("Minecraft")
                                .require_api_version(1, 3, 0);

    if (m_EnableValidationLayers) {
        instance_builder.request_validation_layers().set_debug_callback(Logger::debug_callback);
    }

    const vkb::Instance vkb_inst = instance_builder.build().value();

    m_Instance = vkb_inst.instance;
    m_DeletionQueue.push_function([&] {
        m_Instance.destroy();
    });

    if (m_EnableValidationLayers) {
        m_DebugMessenger = vkb_inst.debug_messenger;
        m_DeletionQueue.push_function([&] {
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
        });
    }

    // Logger::log_available_extensions(vk::enumerateInstanceExtensionProperties().value);
    // Logger::log_available_layers(vk::enumerateInstanceLayerProperties().value);

#pragma endregion

#pragma region Device

    VkSurfaceKHR c_surface;
    glfwCreateWindowSurface(m_Instance, window, nullptr, &c_surface);
    m_Surface = c_surface;

    m_DeletionQueue.push_function([&] {
        m_Instance.destroySurfaceKHR(m_Surface);
    });

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

    m_DeletionQueue.push_function([&] {
        m_Device.destroy();
    });

    Logger::log_device_properties(m_PhysicalDevice);

    m_VkbDevice = vkb_device;

#pragma endregion

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
    m_DeletionQueue.push_function([&] {
        vmaDestroyAllocator(m_Allocator);
    });
}
}