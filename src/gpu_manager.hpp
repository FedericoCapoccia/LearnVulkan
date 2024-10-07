#pragma once
#include <vk_mem_alloc.h>

struct DeletionQueue {
    std::deque<std::function<void()>> Deletors;

    void push_function(std::function<void()>&& function)
    {
        Deletors.push_back(function);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto& Deletor : std::ranges::reverse_view(Deletors)) {
            Deletor(); // call functors
        }
        Deletors.clear();
    }
};

struct GLFWwindow;

namespace Minecraft::VulkanEngine {

/*TODO
 * Singleton and needs initialization
 * provide public function for objects creation and register in a deletion queue called on this object destruction
 *
 */

class GpuManager {
public:
    void init(GLFWwindow* window);
    explicit GpuManager(const bool enable_validation_layers)
        : m_EnableValidationLayers(enable_validation_layers){}
    void destroy() { m_DeletionQueue.flush(); }

    [[nodiscard]] vk::Queue get_queue(const vkb::QueueType queue_type) const
    {
        return m_VkbDevice.get_queue(queue_type).value();
    };

    [[nodiscard]] uint32_t get_queue_index(const vkb::QueueType queue_type) const
    {
        return m_VkbDevice.get_queue_index(queue_type).value();
    };

    vk::Device& device() { return m_Device; }
    vk::PhysicalDevice& physical_device() { return m_PhysicalDevice; }
    vk::SurfaceKHR& surface() { return m_Surface; }
    VmaAllocator& allocator() { return m_Allocator; }

private:
    bool m_EnableValidationLayers;
    DeletionQueue m_DeletionQueue;

    vk::Instance m_Instance { nullptr };
    vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };

    vk::SurfaceKHR m_Surface { nullptr };
    vkb::Device m_VkbDevice;
    vk::Device m_Device { nullptr };
    vk::PhysicalDevice m_PhysicalDevice { nullptr };

    VmaAllocator m_Allocator {};
};

}
