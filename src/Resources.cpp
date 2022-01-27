#include "Resources.h"

#include <ResourceDir.h>

#include <filesystem>
#include <iostream>
#include <optional>

std::optional<std::string> midiplayer_resource_dir(std::string const& path)
{
    std::string rootfile = path + "/.midiplayer-resources";
    return std::filesystem::exists(rootfile) ? path : std::optional<std::string> {};
}

std::string find_resource_path()
{
    auto maybe_path = midiplayer_resource_dir(midiplayer_resource_dir("res").value_or(GLOBAL_RESOURCE_DIR));
    if(!maybe_path.has_value())
    {
        std::cerr << "ERROR: No resource path found. Searched: res, " << GLOBAL_RESOURCE_DIR << std::endl;
        exit(-1);
    }
    return maybe_path.value();
}
