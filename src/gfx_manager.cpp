#include "gfx_manager.hpp"

#include "logger.hpp"
#include "vk_utils.hpp"

namespace Minecraft {

bool check_layers_extensions_support(
    const std::vector<const char*>& extensions,
    const std::vector<const char*>& layers)
{
    const std::vector<vk::ExtensionProperties> supported_extensions = vk::enumerateInstanceExtensionProperties().value;
    Logger::log_available_extensions(supported_extensions);

    for (const auto& extension : extensions) {
        bool found = false;
        for (const auto& supported_extension : supported_extensions) {
            if (strcmp(extension, supported_extension.extensionName) == 0) {
                found = true;
                LOG("Extension: \"{}\" is supported", extension);
            }
        }
        if (!found) {
            LOG("Extension: \"{}\" is not supported", extension);
            return false;
        }
    }

    const std::vector<vk::LayerProperties> supported_layers = vk::enumerateInstanceLayerProperties().value;
    Logger::log_available_layers(supported_layers);

    for (const auto& layer : layers) {
        bool found = false;
        for (const auto& supported_layer : supported_layers) {
            if (strcmp(layer, supported_layer.layerName) == 0) {
                found = true;
                LOG("Layer: \"{}\" is supported", layer);
            }
        }
        if (!found) {
            LOG("Layer: \"{}\" is not supported", layer);
            return false;
        }
    }

    return true;
}

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

    if (!check_layers_extensions_support(extensions, layers)) {
        LOG_ERROR("Device doesn't support required layers and extensions");
        return false;
    }

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