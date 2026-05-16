#include "utils/file_utils.hpp"
#include <sys/stat.h>

namespace to_gif {

std::optional<size_t> file_size(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return static_cast<size_t>(st.st_size);
    }
    return std::nullopt;
}

} // namespace to_gif
