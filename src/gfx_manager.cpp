#include "gfx_manager.hpp"

#include "logger.hpp"
#include "vk_utils.hpp"

namespace Minecraft {

bool GfxManager::init()
{
    assert(m_IsInitialized == false);

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

    m_IsInitialized = true;
    return true;
}


GfxManager::~GfxManager()
{
    LOG("GfxManager destructor");
    m_Instance.destroy(nullptr, m_Dldi);
}

}