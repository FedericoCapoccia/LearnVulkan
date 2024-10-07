#pragma once

namespace Minecraft {

struct DeletionQueue {
    std::deque<std::pair<std::string, std::function<void()>>> Deletors;

    void push_function(const std::string& tag, std::function<void()>&& func)
    {
        Deletors.emplace_back( tag, func );
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto& [tag, func] : std::ranges::reverse_view(Deletors)) {
            func(); // call functors
        }
        Deletors.clear();
    }
};

}

