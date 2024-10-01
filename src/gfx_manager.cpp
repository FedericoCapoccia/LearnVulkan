#include "gfx_manager.hpp"

#include "logger.hpp"

namespace Minecraft {

constexpr bool enable_validation_layers = true;

bool GfxManager::init()
{
    if (m_IsInitialized) {
        LOG_ERROR("Graphics Manager already initialized.");
        return false;
    }

    vkb::InstanceBuilder builder;

    auto instance_builder = builder.set_app_name("Minecraft").require_api_version(1, 3, 0);

    if (enable_validation_layers) {
        instance_builder.request_validation_layers()
            .set_debug_callback(Logger::debug_callback);
    }

    const vkb::Instance vkb_inst = instance_builder.build().value();
    m_Instance = vkb_inst.instance;
    m_DebugMessenger = vkb_inst.debug_messenger;

    m_IsInitialized = true;
    return true;
}

GfxManager::~GfxManager()
{
    LOG("GfxManager destructor");
    if (enable_validation_layers) {
        vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
    }
    m_Instance.destroy();
}

}