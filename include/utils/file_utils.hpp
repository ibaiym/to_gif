#pragma once

#include <cstddef>
#include <optional>
#include <string>

namespace to_gif {

std::optional<size_t> file_size(const std::string& path);

} // namespace to_gif
