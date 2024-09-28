#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

constexpr bool enable_validation_layers = true;
constexpr int32_t WIDTH = 1280;
constexpr int32_t HEIGHT = 720;

int main()
{
#pragma region Window
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Minecraft", nullptr, nullptr);
#pragma endregion

#pragma region Vulkan
    constexpr auto app_info = vk::ApplicationInfo {
        "Minecraft",
        VK_MAKE_VERSION(1, 0, 0),
        "No Engine",
        VK_MAKE_VERSION(1, 0, 0),
        VK_API_VERSION_1_0
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    const auto instance_create_info = vk::InstanceCreateInfo {
        vk::InstanceCreateFlags(),
        &app_info,
        0, {},
        glfwExtensionCount, glfwExtensionNames
    };
    // TODO add layers logic

    const vk::Instance instance = vk::createInstance(instance_create_info);
#pragma endregion

    // while (!glfwWindowShouldClose(window)) {
    // glfwPollEvents();
    // }

    instance.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
