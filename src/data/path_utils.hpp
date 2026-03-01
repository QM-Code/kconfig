#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "json.hpp"

namespace kconfig::common::data::path_utils {

std::filesystem::path Canonicalize(const std::filesystem::path &path);
std::filesystem::path ResolveWithBase(const std::filesystem::path &baseDir, std::string_view value);

enum class JsonReadError {
    None,
    NotFound,
    OpenFailed,
    ParseFailed
};

struct JsonReadResult {
    std::optional<kconfig::common::serialization::Value> json{};
    JsonReadError error = JsonReadError::None;
    std::string message{};
};

JsonReadResult ReadJsonFile(const std::filesystem::path &path);
bool EnsureJsonObjectFile(const std::filesystem::path &path, std::string *error = nullptr);
bool WriteJsonFile(const std::filesystem::path &path,
                   const kconfig::common::serialization::Value &json,
                   std::string *error = nullptr);

void MergeJsonObjects(kconfig::common::serialization::Value &destination,
                      const kconfig::common::serialization::Value &source);

void CollectAssetEntries(const kconfig::common::serialization::Value &node,
                         const std::filesystem::path &baseDir,
                         std::map<std::string, std::filesystem::path> &assetMap,
                         std::string_view prefix = "");

} // namespace kconfig::common::data::path_utils
