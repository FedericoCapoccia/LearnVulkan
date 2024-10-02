#ifndef LOGGER_HPP
#define LOGGER_HPP

#pragma region LogMacros

#ifdef _DEBUG
#define LOG(fmt_str, ...) \
    fmt::print("" fmt_str "\n", ##__VA_ARGS__)
#else
#define LOG(fmt_str, ...)
#endif

#define LOG_ERROR(fmt_str, ...) \
    fmt::print(std::cerr, "[ERROR]\t\t" fmt_str "\n", ##__VA_ARGS__)

#pragma endregion

#define VK_CHECK(x) \
    do { \
        vk::Result res = x;\
        if (res != vk::Result::eSuccess) { \
            LOG_ERROR("Vulkan error: {}", vk::to_string(res)); \
            return false; \
        } \
    } while (0)

namespace Logger {

VKAPI_ATTR inline VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data)
{
    LOG_ERROR("{}", p_callback_data->pMessage);
    return VK_FALSE;
}

inline void log_available_extensions(const std::vector<vk::ExtensionProperties>& supported_extensions)
{
    LOG("Available extensions:");
    for (const auto& extension : supported_extensions) {
        LOG("\t{}", static_cast<const char*>(extension.extensionName));
    }
}

inline void log_available_layers(const std::vector<vk::LayerProperties>& supported_layers)
{
    LOG("Available layers:");
    for (const auto& supported_layer : supported_layers) {
        LOG("\t{}", static_cast<const char*>(supported_layer.layerName));
    }
}

inline void log_device_properties(const vk::PhysicalDevice& device)
{
    vk::PhysicalDeviceProperties properties = device.getProperties();
    std::string device_type = [&]() {
        switch (properties.deviceType) {
        case vk::PhysicalDeviceType::eCpu:
            return "CPU";
        case vk::PhysicalDeviceType::eDiscreteGpu:
            return "Discrete GPU";
        case (vk::PhysicalDeviceType::eIntegratedGpu):
            return "Integrated GPU";
        case (vk::PhysicalDeviceType::eVirtualGpu):
            return "Virtual GPU";
        default:
            return "Other";
        }
    }();

    LOG("Device: {} [{}]", static_cast<const char*>(properties.deviceName), device_type);
}

}

#endif
