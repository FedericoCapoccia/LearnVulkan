#ifndef GFX_MANAGER_HPP
#define GFX_MANAGER_HPP

namespace Minecraft {

class GfxManager {
public:
  bool init();
  ~GfxManager();
private:
  bool m_IsInitialized = false;
  vk::Instance m_Instance { nullptr };
  vk::DebugUtilsMessengerEXT m_DebugMessenger { nullptr };
  //vk::DispatchLoaderDynamic m_Dldi;
  //vk::SurfaceKHR m_Surface;

  //vk::PhysicalDevice m_PhysicalDevice { nullptr };
  //vk::Device m_Device { nullptr };
};

}

#endif
