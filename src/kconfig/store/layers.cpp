#include "../store.hpp"

#include "../data/path_utils.hpp"
#include "api_impl.hpp"
#include "internal.hpp"
#include <kconfig/data/path_resolver.hpp>

#include <ktrace.hpp>
#include <spdlog/spdlog.h>

#include <chrono>
#include <filesystem>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

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

} // namespace

namespace kconfig::store::api {

using internal::g_state;

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

} // namespace kconfig::store::api
