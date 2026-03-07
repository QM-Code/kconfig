#include "../store.hpp"

#include "../data/path_utils.hpp"
#include "api_impl.hpp"
#include "internal.hpp"

#include <ktrace.hpp>

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>

namespace {

using kconfig::store::internal::g_state;

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

} // namespace kconfig::store::api
