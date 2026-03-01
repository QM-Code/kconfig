#include "store.hpp"

#include "data/path_utils.hpp"
#include "data/path_resolver.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <ktrace/trace.hpp>
#include <spdlog/spdlog.h>

namespace {

struct NamedConfigView {
    kconfig::common::serialization::Value json = kconfig::common::serialization::Object();
    std::vector<std::string> sources;
    std::filesystem::path baseDir{};
    bool isMutable = true;
};

struct ConfigStoreState {
    std::mutex mutex;
    bool initialized = false;
    uint64_t revision = 0;
    std::vector<kconfig::common::config::ConfigLayer> defaultLayers;
    std::optional<kconfig::common::config::ConfigLayer> userLayer;
    std::vector<kconfig::common::config::ConfigLayer> runtimeLayers;
    kconfig::common::serialization::Value defaults = kconfig::common::serialization::Object();
    kconfig::common::serialization::Value user = kconfig::common::serialization::Object();
    kconfig::common::serialization::Value merged = kconfig::common::serialization::Object();
    std::unordered_map<std::string, std::filesystem::path> assetLookup;
    std::unordered_map<std::string, std::pair<int, std::size_t>> labelIndex;
    std::filesystem::path userConfigPath;
    std::optional<double> saveIntervalSeconds{};
    std::optional<double> mergeIntervalSeconds{};
    std::chrono::steady_clock::time_point lastSaveTime = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point lastMergeTime = std::chrono::steady_clock::now();
    uint64_t lastSavedRevision = 0;
    bool pendingSave = false;
    bool mergedDirty = false;
    std::map<std::string, kconfig::common::config::ConfigLayer> namedConfigs;
    std::map<std::string, NamedConfigView> namedViews;
    std::map<std::string, std::filesystem::path> namedBackingFiles;
    std::map<std::string, std::filesystem::path> namedAssetRoots;
};

ConfigStoreState g_state;
constexpr std::string_view kLegacyDefaultsName = "__legacy.defaults";
constexpr std::string_view kLegacyUserName = "__legacy.user";
constexpr std::string_view kLegacyMergedName = "__legacy.merged";

bool isLegacyNamedId(std::string_view name) {
    return name == kLegacyDefaultsName || name == kLegacyUserName || name == kLegacyMergedName;
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
    return kconfig::common::data::path_utils::Canonicalize(path);
}

const kconfig::common::serialization::Value *resolvePath(const kconfig::common::serialization::Value &root, std::string_view path) {
    if (path.empty()) {
        return &root;
    }

    const kconfig::common::serialization::Value *current = &root;
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

void roundFloatValues(kconfig::common::serialization::Value &node) {
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

std::optional<double> readIntervalSeconds(const kconfig::common::serialization::Value &root, std::string_view path) {
    const auto *value = resolvePath(root, path);
    if (!value) {
        return std::nullopt;
    }
    if (!value->is_number()) {
        throw std::runtime_error(std::string("Invalid required numeric config: ")
                                 + std::string(path)
                                 + " (must be numeric)");
    }
    const double seconds = value->get<double>();
    if (seconds < 0.0) {
        throw std::runtime_error(std::string("Invalid required numeric config: ")
                                 + std::string(path)
                                 + " (must be >= 0)");
    }
    return std::optional<double>(seconds);
}

std::unordered_map<std::string, std::filesystem::path> buildAssetLookup(
    const std::vector<kconfig::common::config::ConfigLayer> &layers) {
    std::map<std::string, std::filesystem::path> flattened;

    using kconfig::common::data::path_utils::CollectAssetEntries;
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

std::vector<kconfig::common::config::ConfigLayer> loadLayers(const std::vector<kconfig::common::config::ConfigFileSpec> &specs) {
    std::vector<kconfig::common::config::ConfigLayer> layers;
    layers.reserve(specs.size());
    for (const auto &spec : specs) {
        std::filesystem::path path = spec.path;
        if (spec.resolveRelativeToDataRoot && path.is_relative()) {
            path = kconfig::common::data::Resolve(path);
        }
        path = kconfig::common::data::path_utils::Canonicalize(path);
        const std::string label = spec.label.empty() ? path.string() : spec.label;
        KTRACE("config", "loading config file '{}' (label: {})", path.string(), label);
        auto jsonOpt = kconfig::common::data::LoadJsonFile(path, label, spec.missingLevel);
        if (!jsonOpt) {
            if (spec.required) {
                spdlog::error("config_store: Required config missing: {}", path.string());
            }
            continue;
        }
        if (!jsonOpt->is_object()) {
            spdlog::warn("config_store: Config {} is not a JSON object, skipping", path.string());
            continue;
        }
        layers.push_back({std::move(*jsonOpt), path.parent_path(), label});
    }
    return layers;
}

}

namespace kconfig::common::config {

const kconfig::common::serialization::Value *ConfigStore::findNamedValueLocked(std::string_view name) {
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

kconfig::common::serialization::Value *ConfigStore::findMutableNamedValueLocked(std::string_view name) {
    if (isLegacyNamedId(name)) {
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

void ConfigStore::syncLegacyNamedStateLocked() {
    const std::string defaultsName(kLegacyDefaultsName);
    const std::string userName(kLegacyUserName);
    const std::string mergedName(kLegacyMergedName);

    g_state.namedConfigs[defaultsName] = ConfigLayer{
        g_state.defaults,
        {},
        defaultsName,
        false
    };
    g_state.namedConfigs[userName] = ConfigLayer{
        g_state.user,
        g_state.userConfigPath.empty() ? std::filesystem::path{} : g_state.userConfigPath.parent_path(),
        userName,
        true
    };
    g_state.namedViews[mergedName] = NamedConfigView{
        g_state.merged,
        {defaultsName, userName},
        g_state.userConfigPath.empty() ? std::filesystem::path{} : g_state.userConfigPath.parent_path(),
        false
    };

    if (!g_state.userConfigPath.empty()) {
        g_state.namedBackingFiles[userName] = g_state.userConfigPath;
    } else {
        g_state.namedBackingFiles.erase(userName);
    }

    g_state.namedAssetRoots.erase(defaultsName);
    g_state.namedAssetRoots.erase(userName);
    g_state.namedAssetRoots.erase(mergedName);
}

bool ConfigStore::writeJsonFileUnlocked(const std::filesystem::path &path,
                                        const kconfig::common::serialization::Value &json,
                                        std::string *error) {
    if (path.empty()) {
        if (error) {
            *error = "Config path is empty.";
        }
        return false;
    }

    kconfig::common::serialization::Value rounded = json;
    roundFloatValues(rounded);
    KTRACE("config", "writing config '{}'", path.string());
    if (!kconfig::common::data::path_utils::WriteJsonFile(path, rounded, error)) {
        if (error && error->empty()) {
            *error = "Failed to write config file.";
        }
        return false;
    }
    return true;
}

bool ConfigStore::Add(std::string_view name,
                      const kconfig::common::serialization::Value &json,
                      bool isMutable) {
    if (name.empty() || isLegacyNamedId(name)) {
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

bool ConfigStore::Load(std::string_view name,
                       const ConfigFileSpec &spec,
                       bool isMutable) {
    if (name.empty() || isLegacyNamedId(name)) {
        return false;
    }

    std::filesystem::path path = spec.path;
    if (spec.resolveRelativeToDataRoot && path.is_relative()) {
        path = kconfig::common::data::Resolve(path);
    }
    path = canonicalizeFullPath(path);

    const std::string label = spec.label.empty() ? std::string(name) : spec.label;
    auto jsonOpt = kconfig::common::data::LoadJsonFile(path, label, spec.missingLevel);
    if (!jsonOpt) {
        if (spec.required) {
            spdlog::error("config_store: Required named config '{}' missing: {}", name, path.string());
        }
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

bool ConfigStore::Merge(std::string_view targetName,
                        const std::vector<std::string> &sourceNames) {
    if (targetName.empty() || isLegacyNamedId(targetName) || sourceNames.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    kconfig::common::serialization::Value merged = kconfig::common::serialization::Object();
    for (const auto &sourceName : sourceNames) {
        const auto *sourceJson = findNamedValueLocked(sourceName);
        if (!sourceJson) {
            spdlog::warn("config_store: Merge('{}') missing source '{}'", targetName, sourceName);
            return false;
        }
        kconfig::common::data::path_utils::MergeJsonObjects(merged, *sourceJson);
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

bool ConfigStore::Remove(std::string_view name) {
    if (name.empty() || isLegacyNamedId(name)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    const std::string key(name);
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

std::optional<kconfig::common::serialization::Value> ConfigStore::Get(std::string_view name,
                                                                     std::string_view path) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto *root = findNamedValueLocked(name);
    if (!root) {
        return std::nullopt;
    }
    if (isRootPath(path)) {
        return std::optional<kconfig::common::serialization::Value>(std::in_place, *root);
    }
    if (const auto *resolved = resolvePath(*root, path)) {
        return std::optional<kconfig::common::serialization::Value>(std::in_place, *resolved);
    }
    return std::nullopt;
}

bool ConfigStore::Set(std::string_view name,
                      std::string_view path,
                      kconfig::common::serialization::Value value) {
    if (name.empty() || isLegacyNamedId(name)) {
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

bool ConfigStore::Erase(std::string_view name,
                        std::string_view path) {
    if (name.empty() || isLegacyNamedId(name)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto *target = findMutableNamedValueLocked(name);
    if (!target) {
        return false;
    }

    const bool changed = isRootPath(path)
        ? ((*target = kconfig::common::serialization::Object()), true)
        : eraseValueAtPath(*target, path);
    if (!changed) {
        return false;
    }

    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool ConfigStore::SetAssetRoot(std::string_view name,
                               const std::filesystem::path &fullFilesystemPath) {
    if (name.empty()) {
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

bool ConfigStore::SetBackingFile(std::string_view name,
                                 const std::filesystem::path &fullFilesystemPath) {
    if (name.empty()) {
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

bool ConfigStore::RemoveBackingFile(std::string_view name) {
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
        if (auto viewIt = g_state.namedViews.find(key); viewIt != g_state.namedViews.end()) {
            viewIt->second.baseDir = std::filesystem::path{};
        }
    }
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

const std::filesystem::path *ConfigStore::Path(std::string_view name) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.namedBackingFiles.find(std::string(name));
    if (it == g_state.namedBackingFiles.end()) {
        return nullptr;
    }
    return &it->second;
}

bool ConfigStore::Save(std::string_view name, std::string *error) {
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

void ConfigStore::Initialize(const std::vector<ConfigFileSpec> &defaultSpecs,
                             const std::optional<std::filesystem::path> &userConfigPath,
                             const std::vector<ConfigFileSpec> &runtimeSpecs) {
    std::vector<ConfigFileSpec> combinedDefaults = defaultSpecs;

    std::vector<ConfigLayer> defaults = loadLayers(combinedDefaults);
    std::vector<ConfigLayer> runtime = loadLayers(runtimeSpecs);

    std::filesystem::path resolvedUserPath{};
    kconfig::common::serialization::Value userJson = kconfig::common::serialization::Object();
    if (userConfigPath.has_value() && !userConfigPath->empty()) {
        resolvedUserPath = kconfig::common::data::path_utils::Canonicalize(*userConfigPath);
        KTRACE("config", "loading user config '{}'", resolvedUserPath.string());
        if (auto userOpt = kconfig::common::data::LoadJsonFile(resolvedUserPath, "user config", spdlog::level::debug)) {
            if (userOpt->is_object()) {
                userJson = std::move(*userOpt);
            } else {
                spdlog::warn("config_store: User config {} is not a JSON object", resolvedUserPath.string());
            }
        }
    }

    kconfig::common::serialization::Value defaultsMerged = kconfig::common::serialization::Object();
    for (const auto &layer : defaults) {
        kconfig::common::data::path_utils::MergeJsonObjects(defaultsMerged, layer.json);
    }

    std::optional<ConfigLayer> userLayer;
    if (!resolvedUserPath.empty() && userJson.is_object()) {
        userLayer = ConfigLayer{userJson, resolvedUserPath.parent_path(), "user config"};
    }

    std::lock_guard<std::mutex> lock(g_state.mutex);
    g_state.defaultLayers = std::move(defaults);
    g_state.runtimeLayers = std::move(runtime);
    g_state.userLayer = std::move(userLayer);
    g_state.namedConfigs.clear();
    g_state.namedViews.clear();
    g_state.namedBackingFiles.clear();
    g_state.namedAssetRoots.clear();
    g_state.defaults = std::move(defaultsMerged);
    g_state.user = userJson;
    g_state.userConfigPath = resolvedUserPath;
    if (!resolvedUserPath.empty()) {
        g_state.saveIntervalSeconds = readIntervalSeconds(g_state.defaults, "client.config.SaveIntervalSeconds");
        g_state.mergeIntervalSeconds = readIntervalSeconds(g_state.defaults, "client.config.MergeIntervalSeconds");
    } else {
        // Persistence disabled (e.g., server mode): no user-config save/merge intervals apply.
        g_state.saveIntervalSeconds.reset();
        g_state.mergeIntervalSeconds.reset();
    }
    g_state.lastSaveTime = std::chrono::steady_clock::now();
    g_state.lastMergeTime = g_state.lastSaveTime;
    g_state.lastSavedRevision = 0;
    g_state.pendingSave = false;
    g_state.mergedDirty = false;
    rebuildMergedLocked();
    g_state.revision++;
    g_state.lastSavedRevision = g_state.revision;
    g_state.initialized = true;
}

bool ConfigStore::Initialized() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.initialized;
}

uint64_t ConfigStore::Revision() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.revision;
}

const kconfig::common::serialization::Value &ConfigStore::Defaults() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.defaults;
}

const kconfig::common::serialization::Value &ConfigStore::User() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.user;
}

const kconfig::common::serialization::Value &ConfigStore::Merged() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.merged;
}

const kconfig::common::serialization::Value *ConfigStore::Get(std::string_view path) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return nullptr;
    }
    if (g_state.mergedDirty) {
        const auto now = std::chrono::steady_clock::now();
        if (!g_state.mergeIntervalSeconds.has_value() ||
            *g_state.mergeIntervalSeconds <= 0.0 ||
            std::chrono::duration<double>(now - g_state.lastMergeTime).count() >= *g_state.mergeIntervalSeconds) {
            rebuildMergedLocked();
        }
    }
    if (g_state.pendingSave) {
        const auto now = std::chrono::steady_clock::now();
        if (!g_state.saveIntervalSeconds.has_value() ||
            *g_state.saveIntervalSeconds <= 0.0 ||
            std::chrono::duration<double>(now - g_state.lastSaveTime).count() >= *g_state.saveIntervalSeconds) {
            saveUserUnlocked(nullptr, true);
        }
    }
    KTRACE("config.requests", "request for key '{}'", path);
    return resolvePath(g_state.merged, path);
}

std::optional<kconfig::common::serialization::Value> ConfigStore::GetCopy(std::string_view path) {
    if (const auto *value = Get(path)) {
        return std::optional<kconfig::common::serialization::Value>(std::in_place, *value);
    }
    return std::nullopt;
}

bool ConfigStore::Set(std::string_view path, kconfig::common::serialization::Value value) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return false;
    }
    KTRACE("config", "writing key '{}'", path);
    if (!setValueAtPath(g_state.user, path, std::move(value))) {
        return false;
    }
    g_state.userLayer = ConfigLayer{g_state.user, g_state.userConfigPath.parent_path(), "user config"};
    g_state.revision++;
    g_state.mergedDirty = true;
    if (!g_state.mergeIntervalSeconds.has_value() || *g_state.mergeIntervalSeconds <= 0.0) {
        rebuildMergedLocked();
    }
    if (g_state.userConfigPath.empty()) {
        return true;
    }
    g_state.pendingSave = true;
    if (!g_state.saveIntervalSeconds.has_value() || *g_state.saveIntervalSeconds <= 0.0) {
        return saveUserUnlocked(nullptr, true);
    }
    return true;
}

bool ConfigStore::Erase(std::string_view path) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return false;
    }
    KTRACE("config", "erasing key '{}'", path);
    if (!eraseValueAtPath(g_state.user, path)) {
        return false;
    }
    g_state.userLayer = ConfigLayer{g_state.user, g_state.userConfigPath.parent_path(), "user config"};
    g_state.revision++;
    g_state.mergedDirty = true;
    if (!g_state.mergeIntervalSeconds.has_value() || *g_state.mergeIntervalSeconds <= 0.0) {
        rebuildMergedLocked();
    }
    if (g_state.userConfigPath.empty()) {
        return true;
    }
    g_state.pendingSave = true;
    if (!g_state.saveIntervalSeconds.has_value() || *g_state.saveIntervalSeconds <= 0.0) {
        return saveUserUnlocked(nullptr, true);
    }
    return true;
}

bool ConfigStore::ReplaceUserConfig(kconfig::common::serialization::Value userConfig, std::string *error) {
    if (!userConfig.is_object()) {
        userConfig = kconfig::common::serialization::Object();
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return false;
    }
    KTRACE("config", "replacing entire user config");
    g_state.user = std::move(userConfig);
    g_state.userLayer = ConfigLayer{g_state.user, g_state.userConfigPath.parent_path(), "user config"};
    g_state.revision++;
    g_state.mergedDirty = true;
    if (!g_state.mergeIntervalSeconds.has_value() || *g_state.mergeIntervalSeconds <= 0.0) {
        rebuildMergedLocked();
    }
    if (g_state.userConfigPath.empty()) {
        return true;
    }
    g_state.pendingSave = true;
    if (!g_state.saveIntervalSeconds.has_value() || *g_state.saveIntervalSeconds <= 0.0) {
        return saveUserUnlocked(error, true);
    }
    return true;
}

bool ConfigStore::SaveUser(std::string *error) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        if (error) {
            *error = "Config store not initialized.";
        }
        return false;
    }
    if (g_state.userConfigPath.empty()) {
        if (error) {
            *error = "User config persistence is disabled.";
        }
        return false;
    }
    return saveUserUnlocked(error, true);
}

void ConfigStore::Tick() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return;
    }
    if (!g_state.pendingSave) {
        return;
    }
    const auto now = std::chrono::steady_clock::now();
    if (g_state.saveIntervalSeconds.has_value() &&
        *g_state.saveIntervalSeconds > 0.0 &&
        std::chrono::duration<double>(now - g_state.lastSaveTime).count() < *g_state.saveIntervalSeconds) {
        return;
    }
    saveUserUnlocked(nullptr, true);
}

bool ConfigStore::saveUserUnlocked(std::string *error, bool ignoreInterval) {
    if (g_state.userConfigPath.empty()) {
        if (error) {
            *error = "User config persistence is disabled.";
        }
        g_state.pendingSave = false;
        return false;
    }
    const auto now = std::chrono::steady_clock::now();
    if (g_state.revision <= g_state.lastSavedRevision) {
        g_state.pendingSave = false;
        return true;
    }
    if (!ignoreInterval &&
        g_state.saveIntervalSeconds.has_value() &&
        *g_state.saveIntervalSeconds > 0.0 &&
        std::chrono::duration<double>(now - g_state.lastSaveTime).count() < *g_state.saveIntervalSeconds) {
        g_state.pendingSave = true;
        return true;
    }

    const std::filesystem::path path = g_state.userConfigPath;
    if (!writeJsonFileUnlocked(path, g_state.user, error)) {
        return false;
    }
    g_state.lastSaveTime = now;
    g_state.lastSavedRevision = g_state.revision;
    g_state.pendingSave = false;
    return true;
}

const std::filesystem::path &ConfigStore::UserConfigPath() {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    return g_state.userConfigPath;
}

bool ConfigStore::AddRuntimeLayer(const std::string &label,
                                  const kconfig::common::serialization::Value &layerJson,
                                  const std::filesystem::path &baseDir) {
    if (!layerJson.is_object()) {
        spdlog::warn("config_store: Runtime layer '{}' ignored because it is not a JSON object", label);
        return false;
    }
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return false;
    }
    const std::string resolvedLabel = label.empty() ? baseDir.string() : label;
    for (auto &layer : g_state.runtimeLayers) {
        if (layer.label == resolvedLabel) {
            layer.json = layerJson;
            layer.baseDir = baseDir;
            g_state.revision++;
            rebuildMergedLocked();
            return true;
        }
    }
    g_state.runtimeLayers.push_back({layerJson, baseDir, resolvedLabel});
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

bool ConfigStore::RemoveRuntimeLayer(const std::string &label) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    if (!g_state.initialized) {
        return false;
    }
    const auto before = g_state.runtimeLayers.size();
    g_state.runtimeLayers.erase(
        std::remove_if(g_state.runtimeLayers.begin(), g_state.runtimeLayers.end(),
                       [&](const ConfigLayer &layer) { return layer.label == label; }),
        g_state.runtimeLayers.end());
    if (g_state.runtimeLayers.size() == before) {
        return false;
    }
    g_state.revision++;
    rebuildMergedLocked();
    return true;
}

const kconfig::common::serialization::Value *ConfigStore::LayerByLabel(const std::string &label) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    auto it = g_state.labelIndex.find(label);
    if (it == g_state.labelIndex.end()) {
        return nullptr;
    }
    const auto [kind, index] = it->second;
    if (kind == 0) {
        return index < g_state.defaultLayers.size() ? &g_state.defaultLayers[index].json : nullptr;
    }
    if (kind == 1) {
        return g_state.userLayer ? &g_state.userLayer->json : nullptr;
    }
    return index < g_state.runtimeLayers.size() ? &g_state.runtimeLayers[index].json : nullptr;
}

std::filesystem::path ConfigStore::ResolveAssetPath(const std::string &assetKey,
                                                    const std::filesystem::path &defaultPath) {
    std::lock_guard<std::mutex> lock(g_state.mutex);
    const auto it = g_state.assetLookup.find(assetKey);
    if (it != g_state.assetLookup.end()) {
        KTRACE("config", "config_store: resolved asset key '{}' -> '{}'",
                    assetKey,
                    it->second.string());
        return it->second;
    }
    if (defaultPath.empty()) {
        KTRACE("config", "config_store: missing asset key '{}' (no default path)",
                    assetKey);
    } else {
        KTRACE("config", "config_store: missing asset key '{}', using default '{}'",
                    assetKey,
                    defaultPath.string());
    }
    return defaultPath;
}

void ConfigStore::rebuildMergedLocked() {
    g_state.mergedDirty = false;
    g_state.lastMergeTime = std::chrono::steady_clock::now();
    g_state.merged = g_state.defaults;
    if (g_state.userLayer) {
        kconfig::common::data::path_utils::MergeJsonObjects(g_state.merged, g_state.userLayer->json);
    }
    for (const auto &layer : g_state.runtimeLayers) {
        kconfig::common::data::path_utils::MergeJsonObjects(g_state.merged, layer.json);
    }

    syncLegacyNamedStateLocked();

    std::vector<ConfigLayer> allLayers;
    allLayers.reserve(g_state.defaultLayers.size() + g_state.runtimeLayers.size() + 1);
    for (const auto &layer : g_state.defaultLayers) {
        allLayers.push_back(layer);
    }
    if (g_state.userLayer) {
        allLayers.push_back(*g_state.userLayer);
    }
    for (const auto &layer : g_state.runtimeLayers) {
        allLayers.push_back(layer);
    }
    for (const auto &[name, layer] : g_state.namedConfigs) {
        if (isLegacyNamedId(name)) {
            continue;
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
        if (isLegacyNamedId(name)) {
            continue;
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
    g_state.assetLookup = buildAssetLookup(allLayers);

    g_state.labelIndex.clear();
    for (std::size_t i = 0; i < g_state.defaultLayers.size(); ++i) {
        g_state.labelIndex[g_state.defaultLayers[i].label] = {0, i};
    }
    if (g_state.userLayer) {
        g_state.labelIndex[g_state.userLayer->label] = {1, 0};
    }
    for (std::size_t i = 0; i < g_state.runtimeLayers.size(); ++i) {
        g_state.labelIndex[g_state.runtimeLayers[i].label] = {2, i};
    }
}

bool ConfigStore::setValueAtPath(kconfig::common::serialization::Value &root, std::string_view path, kconfig::common::serialization::Value value) {
    std::vector<std::pair<std::string, std::optional<std::size_t>>> segments;
    if (!parsePathSegments(path, segments)) {
        return false;
    }
    if (!root.is_object()) {
        root = kconfig::common::serialization::Object();
    }
    kconfig::common::serialization::Value *current = &root;
    for (std::size_t i = 0; i < segments.size(); ++i) {
        const auto &[key, index] = segments[i];
        const bool last = (i == segments.size() - 1);
        if (!key.empty()) {
            if (!current->is_object()) {
                *current = kconfig::common::serialization::Object();
            }
            if (last && !index.has_value()) {
                (*current)[key] = std::move(value);
                return true;
            }
            if (!current->contains(key)) {
                (*current)[key] = index.has_value() ? kconfig::common::serialization::Array() : kconfig::common::serialization::Object();
            }
            current = &(*current)[key];
        }

        if (index.has_value()) {
            if (!current->is_array()) {
                *current = kconfig::common::serialization::Array();
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

bool ConfigStore::eraseValueAtPath(kconfig::common::serialization::Value &root, std::string_view path) {
    std::vector<std::pair<std::string, std::optional<std::size_t>>> segments;
    if (!parsePathSegments(path, segments)) {
        return false;
    }
    kconfig::common::serialization::Value *current = &root;
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

} // namespace kconfig::common::config
