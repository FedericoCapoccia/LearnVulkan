#pragma once

struct DeletionQueue {
  std::deque<std::function<void()>> Deletors;

  void push_function(std::function<void()>&& function) {
    Deletors.push_back(function);
  }

  void flush() {
    // reverse iterate the deletion queue to execute all the functions
    for (auto & Deletor : std::ranges::reverse_view(Deletors)) {
      Deletor(); //call functors
    }
    Deletors.clear();
  }
};

namespace Minecraft::VulkanEngine {

/*TODO
 * Singleton and needs initialization
 * provide public function for objects creation and register in a deletion queue called on this object destruction
 *
 */

class GpuManager {
public:
  ~GpuManager() { m_DeletionQueue.flush(); }
private:
    DeletionQueue m_DeletionQueue;
    vk::Instance m_Instance;
    vk::Device m_Device;
    vk::PhysicalDevice m_PhysicalDevice;
};

}
