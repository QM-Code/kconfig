#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include <kconfig/json.hpp>

namespace kconfig::data::path_utils {

std::filesystem::path Canonicalize(const std::filesystem::path &path);
std::filesystem::path ResolveWithBase(const std::filesystem::path &baseDir, std::string_view value);

enum class JsonReadError {
    None,
    NotFound,
    OpenFailed,
    ParseFailed
};

struct JsonReadResult {
    std::optional<kconfig::json::Value> json{};
    JsonReadError error = JsonReadError::None;
    std::string message{};
};

JsonReadResult ReadJsonFile(const std::filesystem::path &path);
bool EnsureJsonObjectFile(const std::filesystem::path &path, std::string *error = nullptr);
bool WriteJsonFile(const std::filesystem::path &path,
                   const kconfig::json::Value &json,
                   std::string *error = nullptr);

void MergeJsonObjects(kconfig::json::Value &destination,
                      const kconfig::json::Value &source);

void CollectAssetEntries(const kconfig::json::Value &node,
                         const std::filesystem::path &baseDir,
                         std::map<std::string, std::filesystem::path> &assetMap,
                         std::string_view prefix = "");

} // namespace kconfig::data::path_utils
