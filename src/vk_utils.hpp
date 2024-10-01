#ifndef VK_UTILS_HPP
#define VK_UTILS_HPP

namespace Minecraft::VkUtils {

constexpr bool enable_validation_layers = true;

std::vector<const char*> get_layers();
std::vector<const char*> get_extensions();

constexpr std::array requested_device_extensions = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

}

#endif
