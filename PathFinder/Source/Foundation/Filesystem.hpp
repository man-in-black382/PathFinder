#pragma once

#include <filesystem>

namespace Foundation
{

    template <typename... Ts>
    inline std::filesystem::path PathFromSubpaths(Ts&&... args) 
    {
        std::filesystem::path path;
        ((path /= std::filesystem::path{ std::forward<Ts>(args) }), ...);
        return path;
    }

    inline std::filesystem::path AppendUniquePostfixIfPathExists(const std::filesystem::path& path)
    {
        std::filesystem::path fileName = path.filename();
        std::filesystem::path newPath = path;
        uint64_t id = 1;

        while (std::filesystem::exists(newPath))
        {
            newPath.replace_filename(fileName.string() + "_" + std::to_string(id));
            ++id;
        }

        return newPath;
    }

}

