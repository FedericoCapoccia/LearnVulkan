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

    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Minecraft")
                        .request_validation_layers()
                        .set_debug_callback(Logger::debug_callback)
                        .require_api_version(1, 3, 0)
                        .build();

    const vkb::Instance vkb_inst = inst_ret.value();
    m_Instance = vkb_inst.instance;
    m_DebugMessenger = vkb_inst.debug_messenger;

    m_IsInitialized = true;
    return true;
}

GfxManager::~GfxManager()
{
    LOG("GfxManager destructor");
    if (VkUtils::enable_validation_layers) {
        vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    }
    m_Instance.destroy();
}

}