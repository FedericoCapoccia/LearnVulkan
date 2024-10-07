#pragma once
#include "types.hpp"
#include <vk_mem_alloc.h>

namespace Minecraft::VkEngine {

struct InstanceSpec {
    const char* AppName;
    std::array<uint32_t, 3> ApiVersion;
    bool EnableValidationLayers;
    /*
     * Ignored if validation layers not enabled
     * If validation layers are enabled and this is null a default callback is provided
     */
    std::optional<PFN_vkDebugUtilsMessengerCallbackEXT> DebugCallback;
    std::vector<const char*> Extensions;

    InstanceSpec() = delete;
    InstanceSpec(
        const char* app_name,
        const std::array<uint32_t, 3>& api_version,
        const bool enable_validation_layers,
        const std::optional<PFN_vkDebugUtilsMessengerCallbackEXT>& debug_callback,
        const std::vector<const char*>& extensions)
        : AppName(app_name)
        , ApiVersion(api_version)
        , EnableValidationLayers(enable_validation_layers)
        , DebugCallback(debug_callback)
        , Extensions(extensions)
    {
    }
};

struct DeviceSpec {
    std::array<uint32_t, 2> ApiVersion;
    bool RequireDedicatedComputeQueue;
    bool RequireDedicatedTransferQueue;

    DeviceSpec() = delete;
    DeviceSpec(
        const std::array<uint32_t, 2>& api_version,
        const bool require_dedicated_compute_queue,
        const bool require_dedicated_transfer_queue)
        : ApiVersion(api_version)
        , RequireDedicatedComputeQueue(require_dedicated_compute_queue)
        , RequireDedicatedTransferQueue(require_dedicated_transfer_queue)
    {
    }
};

struct SwapchainSpec {
    GLFWwindow* Window;
    vk::SurfaceFormatKHR Format;
    vk::PresentModeKHR PresentMode;
    vk::ImageUsageFlagBits ImageUsageFlagBits;
    vk::CompositeAlphaFlagBitsKHR CompositeAlphaFlagBits;

    SwapchainSpec() = delete;
    explicit SwapchainSpec(
        GLFWwindow* window,
        const vk::SurfaceFormatKHR& format,
        const vk::PresentModeKHR& present_mode,
        const vk::ImageUsageFlagBits& image_usage_flag_bits,
        const vk::CompositeAlphaFlagBitsKHR& composite_alpha_flag_bits)
        : Window(window)
        , Format(format)
        , PresentMode(present_mode)
        , ImageUsageFlagBits(image_usage_flag_bits)
        , CompositeAlphaFlagBits(composite_alpha_flag_bits)
    {
    }
};

struct GpuManagerSpec {
    const InstanceSpec& InstanceSpec_;
    const DeviceSpec& DeviceSpec_;
    const SwapchainSpec& SwapchainSpec_;
};

struct QueueBundle {
    vk::Queue Queue;
    uint32_t FamilyIndex;
};

struct SwapchainBundle {
    vk::SwapchainKHR Swapchain { nullptr };
    vk::Format ImageFormat {};
    std::vector<VkImage> Images;
    std::vector<VkImageView> ImageViews;
    vk::Extent2D Extent {};
    std::vector<vk::Framebuffer> Framebuffers;
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

struct Buffer {
    vk::Buffer Handle { nullptr };
    VmaAllocation Allocation;
};

class GpuManager {
public:
    GpuManager() { fmt::println("Gpu manager created"); }

    void destroy()
    {
        fmt::println("Gpu manager destructor");
        m_DeletionQueue.flush();
    }

    const SwapchainBundle& init(const GpuManagerSpec& spec);
    const SwapchainBundle& recreate_swapchain(const SwapchainSpec& spec);
    void cleanup_swapchain();
    [[nodiscard]] QueueBundle get_queue(vkb::QueueType queue_type, bool dedicated) const;
    //TODO review
    std::expected<vk::Buffer, const char*> create_vertex_buffer(const std::vector<Vertex>& vertices);

    vk::Device& device() { return m_Device; }
    [[nodiscard]] vk::RenderPass render_pass() const { return m_RenderPass; }

private:

    bool m_Initialized { false };

    DeletionQueue m_DeletionQueue;

    vkb::Instance m_VkbInstance;
    vk::Instance m_Instance { nullptr };
    vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };

    vk::SurfaceKHR m_Surface { nullptr };

    vkb::Device m_VkbDevice;
    vk::Device m_Device { nullptr };
    vk::PhysicalDevice m_PhysicalDevice { nullptr };

    VmaAllocator m_Allocator {};

    SwapchainBundle m_SwapchainBundle;
    DeletionQueue m_SwapchainDeletionQueue;
    vk::RenderPass m_RenderPass;

    Buffer m_VertexBuffer {};

    void init_instance(const InstanceSpec& spec);
    void create_surface(GLFWwindow* window);
    void init_device(const DeviceSpec& spec);
    void init_allocator();

    void create_swapchain(const SwapchainSpec& spec);
    void create_render_pass();
    void create_framebuffers();
};

}
