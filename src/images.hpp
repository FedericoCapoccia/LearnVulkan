#ifndef IMAGES_HPP
#define IMAGES_HPP

namespace Minecraft::VkUtil {

void transition_image(vk::CommandBuffer cmd, vk::Image image, vk::ImageLayout current_layout, vk::ImageLayout new_layout);

}

#endif
