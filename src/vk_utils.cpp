#include "vk_utils.hpp"

namespace Minecraft::VkUtils {

std::vector<const char*> get_layers()
{
    std::vector<const char*> layers {};
    if (enable_validation_layers) {
        layers.emplace_back("VK_LAYER_KHRONOS_validation");
    }
    return layers;
}

std::vector<const char*> get_extensions()
{
    uint32_t glfw_extension_count { 0 };
    const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector extensions(glfw_extensions, glfw_extensions + glfw_extension_count);
    if (enable_validation_layers) {
        extensions.emplace_back("VK_EXT_debug_utils");
    }

    return extensions;
}

}