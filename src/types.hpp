#pragma once
#include <vk_mem_alloc.h>

namespace Minecraft {

struct DeletionQueue {
    std::deque<std::pair<std::string, std::function<void()>>> Deletors;

    void push_function(const std::string& tag, std::function<void()>&& func)
    {
        Deletors.emplace_back(tag, func);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto& [tag, func] : std::ranges::reverse_view(Deletors)) {
            func(); // call functors
        }
        Deletors.clear();
    }
};

struct AllocatedImage {
    vk::Image Image;
    vk::ImageView ImageView;
    VmaAllocation Allocation;
    vk::Extent3D Extent;
    vk::Format Format;
};

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
};

struct DrawImageBundle {
    vk::Image Image;
    vk::ImageView ImageView;
    vk::Extent3D Extent;
    vk::Format Format;
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
    vk::Device DeviceHandle { nullptr };
    DrawImageBundle DrawImage {};

    ResourcesBundle(const vk::Device& device_handle, const DrawImageBundle& draw_image_bundle)
        : DeviceHandle(device_handle)
        , DrawImage(draw_image_bundle)
    {
    }
};

struct PipelineBundle {
    vk::Pipeline Handle { nullptr };
    vk::PipelineLayout Layout { nullptr };
};

}
