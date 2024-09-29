#include "gfx_manager.hpp"
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

    const auto instance_create_info = vk::InstanceCreateInfo {
        vk::InstanceCreateFlags(),
        &app_info,
        static_cast<unsigned int>(layers.size()), layers.data(),
        static_cast<unsigned int>(extensions.size()), extensions.data()
    };

    const auto [res, instance] = vk::createInstance(instance_create_info);
    if (res != vk::Result::eSuccess) {
        std::cerr << "Unable to create instance\n";
        return false;
    }

    m_Instance = { instance };

    m_IsInitialized = true;
    return true;
}


GfxManager::~GfxManager()
{
    std::cout << "GfxManager destructor\n";
    m_Instance.destroy();
}

}