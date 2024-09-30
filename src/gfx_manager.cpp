#include "gfx_manager.hpp"

#include "logger.hpp"
#include "vk_utils.hpp"

namespace Minecraft {

bool GfxManager::init()
{
    if (m_IsInitialized) {
        LOG_ERROR("Graphics Manager already initialized.");
        return false;
    }

#pragma region VkInstance

    constexpr auto app_info = vk::ApplicationInfo {
        "Minecraft",
        VK_MAKE_VERSION(1, 0, 0),
        "No Engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_3
    };

    const std::vector extensions = VkUtils::get_extensions();
    const std::vector layers = VkUtils::get_layers();

    Logger::log_available_extensions();
    Logger::log_available_layers();
    // TODO check if layers and extensions are supported otherwise exit

    const auto instance_create_info = vk::InstanceCreateInfo {
        vk::InstanceCreateFlags(),
        &app_info,
        static_cast<unsigned int>(layers.size()), layers.data(),
        static_cast<unsigned int>(extensions.size()), extensions.data()
    };

    const auto [res, instance] = vk::createInstance(instance_create_info);
    if (res != vk::Result::eSuccess) {
        LOG_ERROR("Unable to create instance");
        return false;
    }

    m_Instance = { instance };
    m_Dldi = vk::DispatchLoaderDynamic(m_Instance, vkGetInstanceProcAddr);

#pragma endregion

#pragma region DebugCallback

    if (VkUtils::enable_validation_layers) {
        const auto message_severity_flag_bits = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
                | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;

        const auto message_type_flags = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
            | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
            | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;

        const auto create_info = vk::DebugUtilsMessengerCreateInfoEXT(
            vk::DebugUtilsMessengerCreateFlagsEXT(),
            message_severity_flag_bits,
            message_type_flags,
            Logger::debug_callback,
            nullptr // user defined data to pass into the callback
        );

        const auto [result, debug_messenger]
            = instance.createDebugUtilsMessengerEXT(create_info, nullptr, m_Dldi);

        if (result != vk::Result::eSuccess) {
            LOG_ERROR("Unable to create debug messenger");
            return false;
        }

        m_DebugMessenger = debug_messenger;
    }

#pragma endregion

    m_IsInitialized = true;
    return true;
}

GfxManager::~GfxManager()
{
    LOG("GfxManager destructor");
    if (VkUtils::enable_validation_layers) {
        m_Instance.destroyDebugUtilsMessengerEXT(m_DebugMessenger, nullptr, m_Dldi);
    }
    m_Instance.destroy(nullptr, m_Dldi);
}

}