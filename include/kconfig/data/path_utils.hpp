#pragma once

#include <filesystem>

namespace kconfig::common::data::path_utils {

std::filesystem::path Canonicalize(const std::filesystem::path &path);

} // namespace kconfig::common::data::path_utils
