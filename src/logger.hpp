#ifndef LOGGER_HPP
#define LOGGER_HPP

#ifdef _DEBUG
    #define LOG(fmt_str, ...) \
        fmt::print(fg(fmt::terminal_color::white), "" fmt_str "\n", ##__VA_ARGS__)
#else
#define LOG(fmt_str, ...)
#endif

#define LOG_ERROR(fmt_str, ...) \
    fmt::print(std::cerr, "[ERROR]\t\t" fmt_str "\n", ##__VA_ARGS__)

namespace Logger {

inline void log_available_extensions()
{
    std::vector<vk::ExtensionProperties> supported_extensions = vk::enumerateInstanceExtensionProperties().value;
    LOG("Available extensions:");
    for (const auto& extension : supported_extensions) {
        LOG("\t{}", static_cast<const char*>(extension.extensionName));
    }
}

inline void log_available_layers()
{
    std::vector<vk::LayerProperties> supported_layers = vk::enumerateInstanceLayerProperties().value;
    LOG("Available layers:");
    for (const auto& supported_layer : supported_layers) {
        LOG("\t{}", static_cast<const char*>(supported_layer.layerName));
    }
}

VKAPI_ATTR inline VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    LOG_ERROR("{}", p_callback_data->pMessage);
    return VK_FALSE;
}


}

#endif
