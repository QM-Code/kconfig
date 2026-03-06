#include "store.hpp"

#include "data/path_utils.hpp"
#include "store/internal.hpp"
#include <kconfig/data/path_resolver.hpp>
#include <kconfig/store.hpp>

#include <ktrace.hpp>
#include <spdlog/spdlog.h>

#include <cctype>
#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace kconfig::store::internal {

ConfigStoreState g_state;

} // namespace kconfig::store::internal

namespace {

using kconfig::store::internal::g_state;

uint64_t nextLayerRevisionLocked(const std::string& name) {
    if (const auto existing = g_state.namedConfigs.find(name); existing != g_state.namedConfigs.end()) {
        return existing->second.revision + 1;
    }
    return 1;
}

std::string DescribeJsonType(const kconfig::json::Value& value) {
    if (value.is_object()) {
        return "object";
    }
    if (value.is_array()) {
        return "array";
    }
    if (value.is_string()) {
        return "string";
    }
    if (value.is_boolean()) {
        return "boolean";
    }
    if (value.is_number_integer()) {
        return "integer";
    }
    if (value.is_number_float()) {
        return "float";
    }
    if (value.is_null()) {
        return "null";
    }
    return "unknown";
}

std::string JoinSourceNames(const std::vector<std::string>& names) {
    std::ostringstream stream;
    bool first = true;
    for (const auto& name : names) {
        if (!first) {
            stream << ", ";
        }
        first = false;
        stream << name;
    }
    return stream.str();
}

std::filesystem::path canonicalizeFullPath(std::filesystem::path path) {
    if (path.empty()) {
        return {};
    }
    if (path.is_relative()) {
        path = std::filesystem::absolute(path);
    }
    return kconfig::data::path_utils::Canonicalize(path);
}

std::string trimWhitespace(std::string_view value) {
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())) != 0) {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())) != 0) {
        value.remove_suffix(1);
    }
    return std::string(value);
}

bool isQuoted(std::string_view value) {
    if (value.size() < 2) {
        return false;
    }
    const char quote = value.front();
    if ((quote != '"' && quote != '\'') || value.back() != quote) {
        return false;
    }
    return true;
}

std::string unquote(std::string_view value) {
    if (!isQuoted(value)) {
        return std::string(value);
    }

    std::string result;
    result.reserve(value.size() - 2);
    bool escaped = false;
    for (std::size_t i = 1; i + 1 < value.size(); ++i) {
        const char ch = value[i];
        if (escaped) {
            result.push_back(ch);
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        result.push_back(ch);
    }
    if (escaped) {
        result.push_back('\\');
    }
    return result;
}

std::size_t findUnquotedChar(std::string_view value, const char target) {
    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool escaped = false;

    for (std::size_t i = 0; i < value.size(); ++i) {
        const char ch = value[i];
        if (escaped) {
            escaped = false;
            continue;
        }
        if ((inSingleQuote || inDoubleQuote) && ch == '\\') {
            escaped = true;
            continue;
        }
        if (!inDoubleQuote && ch == '\'') {
            inSingleQuote = !inSingleQuote;
            continue;
        }
        if (!inSingleQuote && ch == '"') {
            inDoubleQuote = !inDoubleQuote;
            continue;
        }
        if (!inSingleQuote && !inDoubleQuote && ch == target) {
            return i;
        }
    }

    return std::string_view::npos;
}

bool parseCliConfigValue(std::string_view rawValue, kconfig::json::Value& outValue, std::string* error) {
    const std::string text = trimWhitespace(rawValue);
    if (text.empty()) {
        if (error) {
            *error = "config assignment value must not be empty";
        }
        return false;
    }

    if (isQuoted(text) && text.front() == '\'') {
        outValue = kconfig::json::Value(unquote(text));
        return true;
    }

    try {
        outValue = kconfig::json::Parse(text);
        return true;
    } catch (...) {
        if (isQuoted(text)) {
            outValue = kconfig::json::Value(unquote(text));
        } else {
            outValue = kconfig::json::Value(text);
        }
        return true;
    }
}

bool parseCliConfigAssignment(std::string_view text,
                              std::string& outPath,
                              kconfig::json::Value& outValue,
                              std::string* error) {
    const std::string expression = trimWhitespace(text);
    if (expression.empty()) {
        if (error) {
            *error = "config assignment must not be empty";
        }
        return false;
    }

    const std::size_t equal = findUnquotedChar(expression, '=');
    if (equal == std::string::npos) {
        if (error) {
            *error = "config assignment must be '\"<key>\"=<value>'";
        }
        return false;
    }

    const std::string left = trimWhitespace(expression.substr(0, equal));
    const std::string right = trimWhitespace(expression.substr(equal + 1));
    if (left.empty() || right.empty()) {
        if (error) {
            *error = "config assignment must include both key and value";
        }
        return false;
    }
    if (!isQuoted(left)) {
        if (error) {
            *error = "config assignment key must be quoted: '\"<key>\"=<value>'";
        }
        return false;
    }

    outPath = trimWhitespace(unquote(left));
    if (outPath.empty()) {
        if (error) {
            *error = "config assignment key must not be empty";
        }
        return false;
    }

    if (!parseCliConfigValue(right, outValue, error)) {
        return false;
    }

    // Validate key format and path semantics against the store path setter.
    kconfig::json::Value probe = kconfig::json::Object();
    if (!kconfig::store::setValueAtPath(probe, outPath, outValue)) {
        if (error) {
            *error = "config assignment key path is invalid";
        }
        return false;
    }

    return true;
}

} // namespace

namespace kconfig::store {

using internal::g_state;

const kconfig::json::Value* findNamedValueLocked(std::string_view name) {
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end()) {
        return nullptr;
    }
    return &it->second.json;
}

kconfig::json::Value* findMutableNamedValueLocked(std::string_view name) {
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end() || !it->second.isMutable) {
        return nullptr;
    }
    return &it->second.json;
}

static bool AddWithMutability(std::string_view name,
                              const kconfig::json::Value& json,
                              bool isMutable) {
    KTRACE("store.requests",
           "Add namespace='{}' mutable={} json_type='{}'",
           std::string(name),
           isMutable,
           DescribeJsonType(json));
    if (name.empty()) {
        return false;
    }
    if (!json.is_object()) {
        spdlog::warn("config_store: Add('{}') ignored because payload is not a JSON object", name);
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    std::filesystem::path baseDir{};
    if (const auto existing = g_state.namedConfigs.find(key); existing != g_state.namedConfigs.end()) {
        baseDir = existing->second.baseDir;
    }
    if (const auto assetRoot = g_state.namedAssetRoots.find(key); assetRoot != g_state.namedAssetRoots.end()) {
        baseDir = assetRoot->second;
    } else if (const auto backing = g_state.namedBackingFiles.find(key); backing != g_state.namedBackingFiles.end()) {
        baseDir = backing->second.parent_path();
    }
    const uint64_t layerRevision = nextLayerRevisionLocked(key);

    g_state.namedConfigs[key] = ConfigLayer{
        json,
        baseDir,
        key,
        isMutable,
        layerRevision
    };
    if (isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

bool AddMutable(std::string_view name, const kconfig::json::Value& json) {
    return AddWithMutability(name, json, true);
}

bool AddReadOnly(std::string_view name, const kconfig::json::Value& json) {
    return AddWithMutability(name, json, false);
}

bool StoreCliConfig(std::string_view name, std::string_view text, std::string* error) {
    KTRACE("store",
           "StoreCliConfig requested: namespace='{}' text='{}' (enable store.requests for details)",
           std::string(name),
           std::string(text));
    KTRACE("store.requests",
           "StoreCliConfig namespace='{}' text='{}'",
           std::string(name),
           std::string(text));

    if (name.empty()) {
        if (error) {
            *error = "namespace must not be empty";
        }
        return false;
    }
    for (const char ch : name) {
        if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
            if (error) {
                *error = "namespace must not contain whitespace";
            }
            return false;
        }
    }

    std::string path;
    kconfig::json::Value value;
    if (!parseCliConfigAssignment(text, path, value, error)) {
        return false;
    }

    if (!Get(name, ".")) {
        if (!AddMutable(name, kconfig::json::Object())) {
            if (error) {
                *error = "failed to create mutable config namespace '" + std::string(name) + "'";
            }
            return false;
        }
    }

    if (!Set(name, path, std::move(value))) {
        if (error) {
            *error = "failed to set '" + std::string(name) + "." + path + "'";
        }
        return false;
    }

    KTRACE("store.requests",
           "StoreCliConfig namespace='{}' key='{}' -> stored",
           std::string(name),
           path);
    return true;
}

static bool LoadWithMutability(std::string_view name,
                               const std::filesystem::path& filename,
                               bool isMutable) {
    KTRACE("store",
           "Load requested: namespace='{}' mutable={} path='{}' (enable store.requests for details)",
           std::string(name),
           isMutable,
           filename.string());
    KTRACE("store.requests",
           "Load namespace='{}' mutable={} filename='{}'",
           std::string(name),
           isMutable,
           filename.string());
    if (name.empty()) {
        return false;
    }

    std::filesystem::path path = filename;
    if (path.is_relative()) {
        path = kconfig::data::path_resolver::Resolve(path);
    }
    path = canonicalizeFullPath(path);
    KTRACE("store.requests",
           "Load namespace='{}' resolved_path='{}' -> LoadJsonFile",
           std::string(name),
           path.string());

    const std::string label(name);
    auto jsonOpt = kconfig::data::path_resolver::LoadJsonFile(path, label, spdlog::level::warn);
    if (!jsonOpt) {
        return false;
    }
    if (!jsonOpt->is_object()) {
        spdlog::warn("config_store: Named config '{}' from {} is not an object", name, path.string());
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    const auto explicitRootIt = g_state.namedAssetRoots.find(key);
    const std::filesystem::path baseDir =
        (explicitRootIt != g_state.namedAssetRoots.end()) ? explicitRootIt->second : path.parent_path();
    const uint64_t layerRevision = nextLayerRevisionLocked(key);

    g_state.namedConfigs[key] = ConfigLayer{
        std::move(*jsonOpt),
        baseDir,
        key,
        isMutable,
        layerRevision
    };
    g_state.namedBackingFiles[key] = path;
    if (isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.lastSaveTime = std::chrono::steady_clock::now();
        saveState.lastSavedRevision = layerRevision;
        saveState.pendingSave = false;
    } else {
        g_state.namedSaveStates.erase(key);
    }
    g_state.revision++;
    return true;
}

bool LoadMutable(std::string_view name, const std::filesystem::path& filename) {
    return LoadWithMutability(name, filename, true);
}

bool LoadReadOnly(std::string_view name, const std::filesystem::path& filename) {
    return LoadWithMutability(name, filename, false);
}

bool Merge(std::string_view targetName,
           const std::vector<std::string>& sourceNames) {
    KTRACE("store",
           "Merge requested: [{}] -> '{}' (enable store.requests for details)",
           JoinSourceNames(sourceNames),
           std::string(targetName));
    KTRACE("store.requests",
           "Merge target='{}' sources=[{}]",
           std::string(targetName),
           JoinSourceNames(sourceNames));
    if (targetName.empty() || sourceNames.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);

    kconfig::json::Value merged = kconfig::json::Object();
    for (const auto& sourceName : sourceNames) {
        KTRACE("store.requests",
               "Merge target='{}' source='{}' -> lookup",
               std::string(targetName),
               sourceName);
        const auto it = g_state.namedConfigs.find(sourceName);
        if (it == g_state.namedConfigs.end()) {
            spdlog::warn("config_store: Merge('{}') missing source '{}'", targetName, sourceName);
            return false;
        }
        if (!it->second.json.is_object()) {
            spdlog::warn("config_store: Merge('{}') source '{}' is not an object", targetName, sourceName);
            return false;
        }
        kconfig::data::path_utils::MergeJsonObjects(merged, it->second.json);
    }

    const std::string key(targetName);
    std::filesystem::path baseDir{};
    if (const auto existing = g_state.namedConfigs.find(key); existing != g_state.namedConfigs.end()) {
        baseDir = existing->second.baseDir;
    }
    if (const auto explicitRootIt = g_state.namedAssetRoots.find(key); explicitRootIt != g_state.namedAssetRoots.end()) {
        baseDir = explicitRootIt->second;
    } else if (const auto backingIt = g_state.namedBackingFiles.find(key); backingIt != g_state.namedBackingFiles.end()) {
        baseDir = backingIt->second.parent_path();
    }
    const uint64_t layerRevision = nextLayerRevisionLocked(key);

    g_state.namedConfigs[key] = ConfigLayer{
        std::move(merged),
        baseDir,
        key,
        true,
        layerRevision
    };
    auto& saveState = g_state.namedSaveStates[key];
    saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
    g_state.revision++;
    return true;
}

bool Unregister(std::string_view name) {
    KTRACE("store.requests",
           "Unregister namespace='{}'",
           std::string(name));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    const bool removedConfig = (g_state.namedConfigs.erase(key) > 0);
    const bool removedBacking = (g_state.namedBackingFiles.erase(key) > 0);
    const bool removedAssetRoot = (g_state.namedAssetRoots.erase(key) > 0);
    const bool removedSaveState = (g_state.namedSaveStates.erase(key) > 0);
    if (!removedConfig && !removedBacking && !removedAssetRoot && !removedSaveState) {
        return false;
    }

    g_state.revision++;
    return true;
}

bool Delete(std::string_view name) {
    KTRACE("store.requests",
           "Delete namespace='{}'",
           std::string(name));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);

    if (const auto backingIt = g_state.namedBackingFiles.find(key); backingIt != g_state.namedBackingFiles.end()) {
        const auto& backingPath = backingIt->second;
        std::error_code removeEc;
        const bool removedFile = std::filesystem::remove(backingPath, removeEc);
        if (removeEc) {
            spdlog::error("config_store: Delete('{}') failed to remove backing file '{}': {}",
                          name,
                          backingPath.string(),
                          removeEc.message());
            return false;
        }
        if (!removedFile) {
            std::error_code existsEc;
            const bool stillExists = std::filesystem::exists(backingPath, existsEc);
            if (existsEc) {
                spdlog::error("config_store: Delete('{}') failed to verify backing file '{}': {}",
                              name,
                              backingPath.string(),
                              existsEc.message());
                return false;
            }
            if (stillExists) {
                spdlog::error("config_store: Delete('{}') could not remove backing file '{}'",
                              name,
                              backingPath.string());
                return false;
            }
        }
    }

    const bool removedConfig = (g_state.namedConfigs.erase(key) > 0);
    const bool removedBacking = (g_state.namedBackingFiles.erase(key) > 0);
    const bool removedAssetRoot = (g_state.namedAssetRoots.erase(key) > 0);
    const bool removedSaveState = (g_state.namedSaveStates.erase(key) > 0);
    if (!removedConfig && !removedBacking && !removedAssetRoot && !removedSaveState) {
        return false;
    }

    g_state.revision++;
    return true;
}

std::optional<kconfig::json::Value> Get(std::string_view name,
                                        std::string_view path) {
    KTRACE("store.requests",
           "Get namespace='{}' path='{}'",
           std::string(name),
           std::string(path));
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto* root = findNamedValueLocked(name);
    if (!root) {
        KTRACE("store.requests",
               "Get namespace='{}' path='{}' -> miss (namespace not found)",
               std::string(name),
               std::string(path));
        return std::nullopt;
    }
    if (isRootPath(path)) {
        KTRACE("store.requests",
               "Get namespace='{}' path='{}' -> hit root type='{}'",
               std::string(name),
               std::string(path),
               DescribeJsonType(*root));
        return std::optional<kconfig::json::Value>(std::in_place, *root);
    }
    if (const auto* resolved = resolvePath(*root, path)) {
        KTRACE("store.requests",
               "Get namespace='{}' path='{}' -> hit type='{}'",
               std::string(name),
               std::string(path),
               DescribeJsonType(*resolved));
        return std::optional<kconfig::json::Value>(std::in_place, *resolved);
    }
    KTRACE("store.requests",
           "Get namespace='{}' path='{}' -> miss (path not found)",
           std::string(name),
           std::string(path));
    return std::nullopt;
}

bool Set(std::string_view name,
         std::string_view path,
         kconfig::json::Value value) {
    KTRACE("store.requests",
           "Set namespace='{}' path='{}' value_type='{}'",
           std::string(name),
           std::string(path),
           DescribeJsonType(value));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    auto configIt = g_state.namedConfigs.find(key);
    if (configIt == g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return false;
    }
    auto& target = configIt->second.json;

    if (isRootPath(path)) {
        target = std::move(value);
    } else if (!setValueAtPath(target, path, std::move(value))) {
        return false;
    }

    configIt->second.revision++;
    auto& saveState = g_state.namedSaveStates[key];
    saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
    g_state.revision++;
    return true;
}

bool Erase(std::string_view name,
           std::string_view path) {
    KTRACE("store.requests",
           "Erase namespace='{}' path='{}'",
           std::string(name),
           std::string(path));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    auto configIt = g_state.namedConfigs.find(key);
    if (configIt == g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return false;
    }
    auto& target = configIt->second.json;

    const bool changed = isRootPath(path)
        ? ((target = kconfig::json::Object()), true)
        : eraseValueAtPath(target, path);
    if (!changed) {
        return false;
    }

    configIt->second.revision++;
    auto& saveState = g_state.namedSaveStates[key];
    saveState.pendingSave = (g_state.namedBackingFiles.find(key) != g_state.namedBackingFiles.end());
    g_state.revision++;
    return true;
}

bool SetAssetRoot(std::string_view name,
                  const std::filesystem::path& fullFilesystemPath) {
    KTRACE("store.requests",
           "SetAssetRoot namespace='{}' path='{}'",
           std::string(name),
           fullFilesystemPath.string());
    if (name.empty()) {
        return false;
    }

    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    if (canonical.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end()) {
        return false;
    }

    const std::string key(name);
    g_state.namedAssetRoots[key] = canonical;
    g_state.namedConfigs[key].baseDir = canonical;
    g_state.revision++;
    return true;
}

bool SetBackingFile(std::string_view name,
                    const std::filesystem::path& fullFilesystemPath) {
    KTRACE("store.requests",
           "SetBackingFile namespace='{}' path='{}'",
           std::string(name),
           fullFilesystemPath.string());
    if (name.empty()) {
        return false;
    }

    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    if (canonical.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedConfigs.find(std::string(name));
    if (it == g_state.namedConfigs.end()) {
        return false;
    }

    const std::string key(name);
    g_state.namedBackingFiles[key] = canonical;
    if (g_state.namedAssetRoots.find(key) == g_state.namedAssetRoots.end()) {
        g_state.namedConfigs[key].baseDir = canonical.parent_path();
    }
    if (it->second.isMutable) {
        auto& saveState = g_state.namedSaveStates[key];
        saveState.pendingSave = true;
    } else {
        g_state.namedSaveStates.erase(key);
    }

    g_state.revision++;
    return true;
}

bool DetachBackingFile(std::string_view name) {
    KTRACE("store.requests",
           "DetachBackingFile namespace='{}'",
           std::string(name));
    if (name.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    if (g_state.namedBackingFiles.erase(key) == 0) {
        return false;
    }

    if (g_state.namedAssetRoots.find(key) == g_state.namedAssetRoots.end()) {
        if (auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
            configIt->second.baseDir = std::filesystem::path{};
        }
    }
    if (auto saveIt = g_state.namedSaveStates.find(key); saveIt != g_state.namedSaveStates.end()) {
        saveIt->second.pendingSave = false;
    }

    g_state.revision++;
    return true;
}

const std::filesystem::path* BackingFilePath(std::string_view name) {
    KTRACE("store.requests",
           "BackingFilePath namespace='{}'",
           std::string(name));
    thread_local std::filesystem::path snapshot;

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedBackingFiles.find(std::string(name));
    if (it == g_state.namedBackingFiles.end()) {
        return nullptr;
    }

    snapshot = it->second;
    return &snapshot;
}

} // namespace kconfig::store
