#include "store.hpp"

#include "data/path_utils.hpp"
#include <kconfig/data/path_resolver.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <unordered_map>
#include <ktrace/trace.hpp>
#include <spdlog/spdlog.h>

namespace {

struct NamedConfigView {
    kconfig::json::Value json = kconfig::json::Object();
    std::vector<std::string> sources;
    std::filesystem::path baseDir{};
    bool isMutable = true;
};

struct ConfigStoreState {
    std::mutex mutex;
    uint64_t revision = 0;
    std::map<std::string, kconfig::store::ConfigLayer> namedConfigs;
    std::map<std::string, NamedConfigView> namedViews;
    std::map<std::string, std::filesystem::path> namedBackingFiles;
    std::map<std::string, std::filesystem::path> namedAssetRoots;
};

ConfigStoreState g_state;
constexpr std::string_view kDerivedMergedName = "__legacy.merged";
constexpr std::string_view kDerivedAssetsName = "__legacy.assets";
constexpr std::string_view kLegacyDefaultsName = "__legacy.defaults";
constexpr std::string_view kLegacyUserName = "__legacy.user";

bool isReservedNamedId(std::string_view name) {
    return name == kDerivedMergedName
        || name == kDerivedAssetsName
        || name == kLegacyDefaultsName
        || name == kLegacyUserName;
}

bool isRootPath(std::string_view path) {
    return path.empty() || path == ".";
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

const kconfig::json::Value *resolvePath(const kconfig::json::Value &root, std::string_view path) {
    if (path.empty()) {
        return &root;
    }

    const kconfig::json::Value *current = &root;
    std::size_t position = 0;

    while (position < path.size()) {
        const std::size_t dot = path.find('.', position);
        const bool lastSegment = (dot == std::string_view::npos);
        const std::string segment(path.substr(position, lastSegment ? std::string_view::npos : dot - position));
        if (segment.empty()) {
            return nullptr;
        }

        std::string key = segment;
        std::optional<std::size_t> arrayIndex;
        const auto bracketPos = segment.find('[');
        if (bracketPos != std::string::npos) {
            key = segment.substr(0, bracketPos);
            const auto closingPos = segment.find(']', bracketPos);
            if (closingPos == std::string::npos || closingPos != segment.size() - 1) {
                return nullptr;
            }
            const std::string indexText = segment.substr(bracketPos + 1, closingPos - bracketPos - 1);
            if (indexText.empty()) {
                return nullptr;
            }
            try {
                arrayIndex = static_cast<std::size_t>(std::stoul(indexText));
            } catch (...) {
                return nullptr;
            }
        }

        if (!key.empty()) {
            if (!current->is_object()) {
                return nullptr;
            }
            const auto it = current->find(key);
            if (it == current->end()) {
                return nullptr;
            }
            current = &(*it);
        }

        if (arrayIndex.has_value()) {
            if (!current->is_array() || *arrayIndex >= current->size()) {
                return nullptr;
            }
            current = &((*current)[*arrayIndex]);
        }

        if (lastSegment) {
            break;
        }

        position = dot + 1;
    }

    return current;
}

bool parsePathSegments(std::string_view path,
                       std::vector<std::pair<std::string, std::optional<std::size_t>>> &out) {
    out.clear();
    if (path.empty()) {
        return false;
    }
    std::size_t position = 0;
    while (position < path.size()) {
        const std::size_t dot = path.find('.', position);
        const bool lastSegment = (dot == std::string_view::npos);
        const std::string segment(path.substr(position, lastSegment ? std::string_view::npos : dot - position));
        if (segment.empty()) {
            return false;
        }
        std::string key = segment;
        std::optional<std::size_t> index;
        const auto bracketPos = segment.find('[');
        if (bracketPos != std::string::npos) {
            key = segment.substr(0, bracketPos);
            const auto closingPos = segment.find(']', bracketPos);
            if (closingPos == std::string::npos || closingPos != segment.size() - 1) {
                return false;
            }
            const std::string indexText = segment.substr(bracketPos + 1, closingPos - bracketPos - 1);
            if (indexText.empty()) {
                return false;
            }
            try {
                index = static_cast<std::size_t>(std::stoul(indexText));
            } catch (...) {
                return false;
            }
        }
        out.emplace_back(std::move(key), index);
        if (lastSegment) {
            break;
        }
        position = dot + 1;
    }
    return !out.empty();
}

void roundFloatValues(kconfig::json::Value &node) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            roundFloatValues(it.value());
        }
        return;
    }
    if (node.is_array()) {
        for (auto &entry : node) {
            roundFloatValues(entry);
        }
        return;
    }
    if (node.is_number_float()) {
        const double value = node.get<double>();
        const double rounded = std::round(value * 100.0) / 100.0;
        node = rounded;
    }
}

std::unordered_map<std::string, std::filesystem::path> buildAssetLookup(
    const std::vector<kconfig::store::ConfigLayer> &layers) {
    std::map<std::string, std::filesystem::path> flattened;

    using kconfig::data::path_utils::CollectAssetEntries;
    std::unordered_map<std::string, std::filesystem::path> lookup;
    for (const auto &layer : layers) {
        if (!layer.json.is_object()) {
            continue;
        }
        const auto assetsIt = layer.json.find("assets");
        if (assetsIt != layer.json.end() && assetsIt->is_object()) {
            CollectAssetEntries(*assetsIt, layer.baseDir, flattened, "assets");
        }
        const auto fontsIt = layer.json.find("fonts");
        if (fontsIt != layer.json.end() && fontsIt->is_object()) {
            CollectAssetEntries(*fontsIt, layer.baseDir, flattened, "fonts");
        }
    }

    lookup.reserve(flattened.size());
    for (const auto &[key, resolvedPath] : flattened) {
        lookup[key] = resolvedPath;
    }
    return lookup;
}

}

namespace kconfig::store {

const kconfig::json::Value *findNamedValueLocked(std::string_view name) {
    const std::string key(name);
    const auto configIt = g_state.namedConfigs.find(key);
    if (configIt != g_state.namedConfigs.end()) {
        return &configIt->second.json;
    }
    const auto viewIt = g_state.namedViews.find(key);
    if (viewIt != g_state.namedViews.end()) {
        return &viewIt->second.json;
    }
    return nullptr;
}

kconfig::json::Value *findMutableNamedValueLocked(std::string_view name) {
    if (isReservedNamedId(name)) {
        return nullptr;
    }
    const std::string key(name);
    const auto configIt = g_state.namedConfigs.find(key);
    if (configIt != g_state.namedConfigs.end()) {
        return configIt->second.isMutable ? &configIt->second.json : nullptr;
    }
    const auto viewIt = g_state.namedViews.find(key);
    if (viewIt != g_state.namedViews.end()) {
        return viewIt->second.isMutable ? &viewIt->second.json : nullptr;
    }
    return nullptr;
}

bool writeJsonFileUnlocked(const std::filesystem::path &path,
                                        const kconfig::json::Value &json,
                                        std::string *error) {
    if (path.empty()) {
        if (error) {
            *error = "Config path is empty.";
        }
        return false;
    }

    kconfig::json::Value rounded = json;
    roundFloatValues(rounded);
    KTRACE("config", "writing config '{}'", path.string());
    if (!kconfig::data::path_utils::WriteJsonFile(path, rounded, error)) {
        if (error && error->empty()) {
            *error = "Failed to write config file.";
        }
        return false;
    }
    return true;
}

static bool AddWithMutability(std::string_view name,
                              const kconfig::json::Value &json,
                              bool isMutable) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }
    if (!json.is_object()) {
        spdlog::warn("config_store: Add('{}') ignored because payload is not a JSON object", name);
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    std::filesystem::path existingBase{};
    if (const auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
        existingBase = configIt->second.baseDir;
    } else if (const auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
        existingBase = viewIt->second.baseDir;
    }

    g_state.namedViews.erase(key);
    g_state.namedConfigs[key] = ConfigLayer{
        json,
        existingBase,
        key,
        isMutable
    };
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool AddMutable(std::string_view name, const kconfig::json::Value &json) {
    return AddWithMutability(name, json, true);
}

bool AddReadOnly(std::string_view name, const kconfig::json::Value &json) {
    return AddWithMutability(name, json, false);
}

static bool LoadWithMutability(std::string_view name,
                               const std::filesystem::path &filename,
                               bool isMutable) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }

    std::filesystem::path path = filename;
    if (path.is_relative()) {
        path = kconfig::data::path_resolver::Resolve(path);
    }
    path = canonicalizeFullPath(path);

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
    g_state.namedViews.erase(key);
    const auto explicitRootIt = g_state.namedAssetRoots.find(key);
    const std::filesystem::path baseDir =
        (explicitRootIt != g_state.namedAssetRoots.end()) ? explicitRootIt->second : path.parent_path();
    g_state.namedConfigs[key] = ConfigLayer{
        std::move(*jsonOpt),
        baseDir,
        key,
        isMutable
    };
    g_state.namedBackingFiles[key] = path;
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool LoadMutable(std::string_view name, const std::filesystem::path &filename) {
    return LoadWithMutability(name, filename, true);
}

bool LoadReadOnly(std::string_view name, const std::filesystem::path &filename) {
    return LoadWithMutability(name, filename, false);
}

bool Merge(std::string_view targetName,
                        const std::vector<std::string> &sourceNames) {
    if (targetName.empty() || isReservedNamedId(targetName) || sourceNames.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    kconfig::json::Value merged = kconfig::json::Object();
    for (const auto &sourceName : sourceNames) {
        const auto *sourceJson = findNamedValueLocked(sourceName);
        if (!sourceJson) {
            spdlog::warn("config_store: Merge('{}') missing source '{}'", targetName, sourceName);
            return false;
        }
        kconfig::data::path_utils::MergeJsonObjects(merged, *sourceJson);
    }

    const std::string key(targetName);
    std::filesystem::path existingBase{};
    if (const auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
        existingBase = configIt->second.baseDir;
    } else if (const auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
        existingBase = viewIt->second.baseDir;
    }

    const auto explicitRootIt = g_state.namedAssetRoots.find(key);
    if (explicitRootIt != g_state.namedAssetRoots.end()) {
        existingBase = explicitRootIt->second;
    } else if (const auto backingIt = g_state.namedBackingFiles.find(key); backingIt != g_state.namedBackingFiles.end()) {
        existingBase = backingIt->second.parent_path();
    }

    g_state.namedConfigs.erase(key);
    g_state.namedViews[key] = NamedConfigView{
        std::move(merged),
        sourceNames,
        existingBase,
        true
    };
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

static bool unregisterNamedConfigLocked(const std::string &key) {
    const bool removedConfig = (g_state.namedConfigs.erase(key) > 0);
    const bool removedView = (g_state.namedViews.erase(key) > 0);
    const bool removedBacking = (g_state.namedBackingFiles.erase(key) > 0);
    const bool removedAssetRoot = (g_state.namedAssetRoots.erase(key) > 0);
    const bool removed = removedConfig || removedView || removedBacking || removedAssetRoot;
    if (!removed) {
        return false;
    }

    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool Unregister(std::string_view name) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    return unregisterNamedConfigLocked(std::string(name));
}

bool Delete(std::string_view name) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    if (const auto backingIt = g_state.namedBackingFiles.find(key); backingIt != g_state.namedBackingFiles.end()) {
        const auto &backingPath = backingIt->second;
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
    return unregisterNamedConfigLocked(key);
}

std::optional<kconfig::json::Value> Get(std::string_view name,
                                                                     std::string_view path) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto *root = findNamedValueLocked(name);
    if (!root) {
        return std::nullopt;
    }
    if (isRootPath(path)) {
        return std::optional<kconfig::json::Value>(std::in_place, *root);
    }
    if (const auto *resolved = resolvePath(*root, path)) {
        return std::optional<kconfig::json::Value>(std::in_place, *resolved);
    }
    return std::nullopt;
}

bool Set(std::string_view name,
                      std::string_view path,
                      kconfig::json::Value value) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto *target = findMutableNamedValueLocked(name);
    if (!target) {
        return false;
    }

    if (isRootPath(path)) {
        *target = std::move(value);
    } else if (!setValueAtPath(*target, path, std::move(value))) {
        return false;
    }

    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool Erase(std::string_view name,
                        std::string_view path) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto *target = findMutableNamedValueLocked(name);
    if (!target) {
        return false;
    }

    const bool changed = isRootPath(path)
        ? ((*target = kconfig::json::Object()), true)
        : eraseValueAtPath(*target, path);
    if (!changed) {
        return false;
    }

    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool SetAssetRoot(std::string_view name,
                               const std::filesystem::path &fullFilesystemPath) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }
    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    if (canonical.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!findNamedValueLocked(name)) {
        return false;
    }
    const std::string key(name);
    g_state.namedAssetRoots[key] = canonical;
    if (auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
        configIt->second.baseDir = canonical;
    }
    if (auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
        viewIt->second.baseDir = canonical;
    }
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool SetBackingFile(std::string_view name,
                                 const std::filesystem::path &fullFilesystemPath) {
    if (name.empty() || isReservedNamedId(name)) {
        return false;
    }
    const std::filesystem::path canonical = canonicalizeFullPath(fullFilesystemPath);
    if (canonical.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!findNamedValueLocked(name)) {
        return false;
    }
    const std::string key(name);
    g_state.namedBackingFiles[key] = canonical;
    if (g_state.namedAssetRoots.find(key) == g_state.namedAssetRoots.end()) {
        const std::filesystem::path fallbackRoot = canonical.parent_path();
        if (auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
            configIt->second.baseDir = fallbackRoot;
        }
        if (auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
            viewIt->second.baseDir = fallbackRoot;
        }
    }
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool DetachBackingFile(std::string_view name) {
    if (name.empty() || isReservedNamedId(name)) {
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
        if (auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
            viewIt->second.baseDir = std::filesystem::path{};
        }
    }
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

const std::filesystem::path *BackingFilePath(std::string_view name) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedBackingFiles.find(std::string(name));
    if (it == g_state.namedBackingFiles.end()) {
        return nullptr;
    }
    return &it->second;
}

bool WriteBackingFile(std::string_view name, std::string *error) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto *value = findNamedValueLocked(name);
    if (!value) {
        if (error) {
            *error = "Named config not found.";
        }
        return false;
    }
    const auto it = g_state.namedBackingFiles.find(std::string(name));
    if (it == g_state.namedBackingFiles.end()) {
        if (error) {
            *error = "No backing file configured.";
        }
        return false;
    }
    return writeJsonFileUnlocked(it->second, *value, error);
}

bool ReloadBackingFile(std::string_view name, std::string *error) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
    const auto backingIt = g_state.namedBackingFiles.find(key);
    if (backingIt == g_state.namedBackingFiles.end()) {
        if (error) {
            *error = "No backing file configured.";
        }
        return false;
    }

    const std::filesystem::path path = backingIt->second;
    const auto readResult = kconfig::data::path_utils::ReadJsonFile(path);
    if (!readResult.json.has_value()) {
        if (error) {
            switch (readResult.error) {
            case kconfig::data::path_utils::JsonReadError::NotFound:
                *error = "Backing file not found: " + path.string();
                break;
            case kconfig::data::path_utils::JsonReadError::OpenFailed:
                *error = "Failed to open backing file: " + path.string();
                break;
            case kconfig::data::path_utils::JsonReadError::ParseFailed:
                *error = "Failed to parse backing file: " + readResult.message;
                break;
            case kconfig::data::path_utils::JsonReadError::None:
                *error = "Failed to read backing file.";
                break;
            }
        }
        return false;
    }

    if (!readResult.json->is_object()) {
        if (error) {
            *error = "Backing file must contain a JSON object.";
        }
        return false;
    }

    kconfig::json::Value loaded = std::move(*readResult.json);
    if (auto configIt = g_state.namedConfigs.find(key); configIt != g_state.namedConfigs.end()) {
        configIt->second.json = std::move(loaded);
    } else if (auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
        viewIt->second.json = std::move(loaded);
    } else {
        if (error) {
            *error = "Named config not found.";
        }
        return false;
    }

    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

void rebuildMergedLocked() {
    g_state.namedConfigs.erase(std::string(kDerivedMergedName));
    g_state.namedConfigs.erase(std::string(kDerivedAssetsName));
    g_state.namedConfigs.erase(std::string(kLegacyDefaultsName));
    g_state.namedConfigs.erase(std::string(kLegacyUserName));

    kconfig::json::Value merged = kconfig::json::Object();
    std::vector<ConfigLayer> allLayers;
    allLayers.reserve(g_state.namedConfigs.size() + g_state.namedViews.size());
    for (const auto &[name, layer] : g_state.namedConfigs) {
        if (isReservedNamedId(name)) {
            continue;
        }

        if (layer.json.is_object()) {
            kconfig::data::path_utils::MergeJsonObjects(merged, layer.json);
        }

        ConfigLayer resolved = layer;
        if (const auto rootIt = g_state.namedAssetRoots.find(name); rootIt != g_state.namedAssetRoots.end()) {
            resolved.baseDir = rootIt->second;
        } else if (const auto backingIt = g_state.namedBackingFiles.find(name); backingIt != g_state.namedBackingFiles.end()) {
            resolved.baseDir = backingIt->second.parent_path();
        }
        allLayers.push_back(std::move(resolved));
    }
    for (const auto &[name, view] : g_state.namedViews) {
        if (isReservedNamedId(name)) {
            continue;
        }

        if (view.json.is_object()) {
            kconfig::data::path_utils::MergeJsonObjects(merged, view.json);
        }

        std::filesystem::path resolvedBase = view.baseDir;
        if (const auto rootIt = g_state.namedAssetRoots.find(name); rootIt != g_state.namedAssetRoots.end()) {
            resolvedBase = rootIt->second;
        } else if (const auto backingIt = g_state.namedBackingFiles.find(name); backingIt != g_state.namedBackingFiles.end()) {
            resolvedBase = backingIt->second.parent_path();
        }
        allLayers.push_back(ConfigLayer{
            view.json,
            resolvedBase,
            name,
            view.isMutable
        });
    }

    kconfig::json::Value assetIndex = kconfig::json::Object();
    const auto assetLookup = buildAssetLookup(allLayers);
    for (const auto &[key, resolvedPath] : assetLookup) {
        // Keys are already dot-separated paths (e.g. "assets.foo.bar"), so reuse setValueAtPath.
        (void)setValueAtPath(assetIndex, key, resolvedPath.string());
    }

    const std::string mergedName(kDerivedMergedName);
    g_state.namedViews[mergedName] = NamedConfigView{
        std::move(merged),
        {},
        {},
        false
    };

    const std::string assetsName(kDerivedAssetsName);
    g_state.namedViews[assetsName] = NamedConfigView{
        std::move(assetIndex),
        {},
        {},
        false
    };

    g_state.namedBackingFiles.erase(mergedName);
    g_state.namedBackingFiles.erase(assetsName);
    g_state.namedAssetRoots.erase(mergedName);
    g_state.namedAssetRoots.erase(assetsName);
    g_state.namedAssetRoots.erase(std::string(kLegacyDefaultsName));
    g_state.namedAssetRoots.erase(std::string(kLegacyUserName));
}

bool setValueAtPath(kconfig::json::Value &root, std::string_view path, kconfig::json::Value value) {
    std::vector<std::pair<std::string, std::optional<std::size_t>>> segments;
    if (!parsePathSegments(path, segments)) {
        return false;
    }
    if (!root.is_object()) {
        root = kconfig::json::Object();
    }
    kconfig::json::Value *current = &root;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto &[key, index] = segments[i];
        const bool last = (i == segments.size() - 1);
        if (!key.empty()) {
            if (!current->is_object()) {
                *current = kconfig::json::Object();
            }
            if (last && !index.has_value()) {
                (*current)[key] = std::move(value);
                return true;
            }
            if (!current->contains(key)) {
                (*current)[key] = index.has_value() ? kconfig::json::Array() : kconfig::json::Object();
            }
            current = &(*current)[key];
        }

        if (index.has_value()) {
            if (!current->is_array()) {
                *current = kconfig::json::Array();
            }
            while (current->size() <= *index) {
                current->push_back(nullptr);
            }
            if (last) {
                (*current)[*index] = std::move(value);
                return true;
            }
            current = &(*current)[*index];
        }
    }
    return false;
}

bool eraseValueAtPath(kconfig::json::Value &root, std::string_view path) {
    std::vector<std::pair<std::string, std::optional<std::size_t>>> segments;
    if (!parsePathSegments(path, segments)) {
        return false;
    }
    kconfig::json::Value *current = &root;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto &[key, index] = segments[i];
        const bool last = (i == segments.size() - 1);
        if (!key.empty()) {
            if (!current->is_object()) {
                return false;
            }
            auto it = current->find(key);
            if (it == current->end()) {
                return false;
            }
            if (last && !index.has_value()) {
                current->erase(key);
                return true;
            }
            current = &(*it);
        }
        if (index.has_value()) {
            if (!current->is_array() || *index >= current->size()) {
                return false;
            }
            if (last) {
                (*current)[*index] = nullptr;
                return true;
            }
            current = &(*current)[*index];
        }
    }
    return false;
}

} // namespace kconfig::store
