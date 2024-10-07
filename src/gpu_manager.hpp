#pragma once
#include "types.hpp"
#include <vk_mem_alloc.h>

#include <utility>

namespace Minecraft::VkEngine {

struct QueueBundle {
    vk::Queue Queue;
    uint32_t FamilyIndex;
};

struct SwapchainBundle {
    vk::SwapchainKHR Handle;
    vk::Format ImageFormat {};
    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    vk::Extent2D Extent {};
    vk::Extent2D DrawExtent {};
};

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription get_binding_description()
    {
        return vk::VertexInputBindingDescription {
            0, sizeof(Vertex), vk::VertexInputRate::eVertex
        };
    }

    static std::array<vk::VertexInputAttributeDescription, 2> get_attribute_descriptions()
    {
        return {
            vk::VertexInputAttributeDescription {
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos) },

            vk::VertexInputAttributeDescription {
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) }
        };
    }
};

struct GpuManagerSpec {
    const char* AppName { "Default Application Name" };
    bool EnableValidation { true };
    std::optional<PFN_vkDebugUtilsMessengerCallbackEXT> DebugCallback;
    GLFWwindow* Window { nullptr };

    GpuManagerSpec(const char* const app_name, const bool enable_validation, const std::optional<PFN_vkDebugUtilsMessengerCallbackEXT>& debug_callback, GLFWwindow* const window)
        : AppName(app_name)
        , EnableValidation(enable_validation)
        , DebugCallback(debug_callback)
        , Window(window)
    {
    }
};

struct ResourcesBundle {
    QueueBundle GraphicsQueue {};
    SwapchainBundle Swapchain {};
    vk::Device DeviceHandle { nullptr };

    ResourcesBundle(const QueueBundle& queue, SwapchainBundle swapchain, const vk::Device& device_handle)
        : GraphicsQueue(queue)
        , Swapchain(std::move(swapchain))
        , DeviceHandle(device_handle)
    {
    }
};

class GpuManager {
public:
    GpuManager() { fmt::println("Gpu manager created"); }

    void destroy_swapchain();
    void destroy()
    {
        fmt::println("GpuManager destructor");
        m_DeletionQueue.flush();
        m_Initialized = false;
    }

    ResourcesBundle init(const GpuManagerSpec& spec);
    void cleanup_swapchain();

    vk::Device& device() { return m_Device; }

private:
    bool m_Initialized { false };
    DeletionQueue m_DeletionQueue;

    // Core
    vk::Extent2D m_WindowExtent;
    vk::Instance m_Instance { nullptr };
    vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };
    vk::SurfaceKHR m_Surface { nullptr };
    vk::Device m_Device { nullptr };
    vk::PhysicalDevice m_PhysicalDevice { nullptr };
    VmaAllocator m_Allocator {};

    SwapchainBundle m_SwapchainBundle;
    // DeletionQueue m_SwapchainDeletionQueue;

    void create_swapchain(uint32_t width, uint32_t height);
    void init_swapchain();
};

}
