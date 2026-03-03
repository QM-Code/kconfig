#pragma once

#include <filesystem>

namespace kconfig::data::path_utils {

std::filesystem::path Canonicalize(const std::filesystem::path &path);

} // namespace kconfig::data::path_utils
