#include "internal.hpp"

#include "../data/path_utils.hpp"
#include <kconfig/data/path_resolver.hpp>
#include <kconfig/store.hpp>

#include <ktrace.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

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

std::string DescribeSaveInterval(const std::optional<double>& seconds) {
    if (!seconds.has_value()) {
        return "unset";
    }
    std::ostringstream stream;
    stream << *seconds << "s";
    return stream.str();
}

void roundFloatValues(kconfig::json::Value& node) {
    if (node.is_object()) {
        for (auto it = node.begin(); it != node.end(); ++it) {
            roundFloatValues(it.value());
        }
        return;
    }

    if (node.is_array()) {
        for (auto& entry : node) {
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

constexpr std::string_view kUserConfigFilename = "config.json";

const char* UserConfigLoadModeName(kconfig::store::UserConfigLoadMode mode) {
    switch (mode) {
    case kconfig::store::UserConfigLoadMode::ReadOnly:
        return "readonly";
    case kconfig::store::UserConfigLoadMode::Mutable:
        return "mutable";
    }
    return "unknown";
}

std::filesystem::path CurrentUserConfigFilePath() {
    {
        std::lock_guard<std::mutex> lock(kconfig::store::internal::g_state.mutex);
        if (kconfig::store::internal::g_state.userConfigFilePathOverride.has_value()) {
            return *kconfig::store::internal::g_state.userConfigFilePathOverride;
        }
    }
    return kconfig::data::path_resolver::UserConfigDirectory() / std::string(kUserConfigFilename);
}

} // namespace

namespace kconfig::store {

bool SetUserConfigFilePath(const std::filesystem::path& fullFilesystemPath) {
    KTRACE("store",
           "SetUserConfigFilePath requested: path='{}' (enable store.requests for details)",
           fullFilesystemPath.string());
    KTRACE("store.requests",
           "SetUserConfigFilePath path='{}'",
           fullFilesystemPath.string());

    if (fullFilesystemPath.empty()
        || !fullFilesystemPath.has_filename()
        || fullFilesystemPath.filename() == "."
        || fullFilesystemPath.filename() == "..") {
        return false;
    }

    const std::filesystem::path canonical = kconfig::data::path_utils::Canonicalize(fullFilesystemPath);

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    internal::g_state.userConfigFilePathOverride = canonical;
    return true;
}

bool writeJsonFileUnlocked(const std::filesystem::path& path,
                           const kconfig::json::Value& json,
                           std::string* error) {
    KTRACE("store.requests",
           "WriteJsonFile path='{}' json_type='{}'",
           path.string(),
           DescribeJsonType(json));
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

bool WriteBackingFile(std::string_view name, std::string* error) {
    KTRACE("store",
           "WriteBackingFile requested: namespace='{}' (enable store.requests for details)",
           std::string(name));
    KTRACE("store.requests",
           "WriteBackingFile namespace='{}'",
           std::string(name));
    std::string key;
    std::filesystem::path path;
    kconfig::json::Value value;
    uint64_t revisionSnapshot = 0;
    bool isMutable = false;

    {
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        key = std::string(name);
        const auto configIt = internal::g_state.namedConfigs.find(key);
        if (configIt == internal::g_state.namedConfigs.end()) {
            if (error) {
                *error = "Named config not found.";
            }
            return false;
        }

        const auto backing = internal::g_state.namedBackingFiles.find(key);
        if (backing == internal::g_state.namedBackingFiles.end()) {
            if (error) {
                *error = "No backing file configured.";
            }
            return false;
        }

        value = configIt->second.json;
        revisionSnapshot = configIt->second.revision;
        isMutable = configIt->second.isMutable;
        path = backing->second;
    }

    if (!writeJsonFileUnlocked(path, value, error)) {
        return false;
    }

    if (!isMutable) {
        return true;
    }

    const auto saveTime = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const auto configIt = internal::g_state.namedConfigs.find(key);
    const auto saveIt = internal::g_state.namedSaveStates.find(key);
    const auto backingIt = internal::g_state.namedBackingFiles.find(key);
    if (configIt == internal::g_state.namedConfigs.end()
        || saveIt == internal::g_state.namedSaveStates.end()
        || backingIt == internal::g_state.namedBackingFiles.end()) {
        return true;
    }

    auto& saveState = saveIt->second;
    saveState.lastSaveTime = saveTime;
    saveState.lastSavedRevision = revisionSnapshot;
    saveState.pendingSave = (configIt->second.revision != revisionSnapshot) || (backingIt->second != path);
    return true;
}

bool ReloadBackingFile(std::string_view name, std::string* error) {
    KTRACE("store",
           "ReloadBackingFile requested: namespace='{}' (enable store.requests for details)",
           std::string(name));
    KTRACE("store.requests",
           "ReloadBackingFile namespace='{}'",
           std::string(name));
    std::filesystem::path path;

    {
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        const auto backingIt = internal::g_state.namedBackingFiles.find(std::string(name));
        if (backingIt == internal::g_state.namedBackingFiles.end()) {
            if (error) {
                *error = "No backing file configured.";
            }
            return false;
        }
        path = backingIt->second;
    }

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

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const std::string key(name);
    auto it = internal::g_state.namedConfigs.find(key);
    if (it == internal::g_state.namedConfigs.end()) {
        if (error) {
            *error = "Named config not found.";
        }
        return false;
    }

    it->second.json = std::move(*readResult.json);
    it->second.revision++;
    if (it->second.isMutable) {
        auto& saveState = internal::g_state.namedSaveStates[key];
        saveState.lastSaveTime = std::chrono::steady_clock::now();
        saveState.lastSavedRevision = it->second.revision;
        saveState.pendingSave = false;
    } else {
        internal::g_state.namedSaveStates.erase(key);
    }
    internal::g_state.revision++;
    return true;
}

bool SetUserConfigDirname(std::string_view dirname) {
    KTRACE("store",
           "SetUserConfigDirname requested: dirname='{}' (enable store.requests for details)",
           std::string(dirname));
    KTRACE("store.requests",
           "SetUserConfigDirname dirname='{}'",
           std::string(dirname));
    std::filesystem::path dirnamePath(dirname);
    if (dirnamePath.empty()
        || dirnamePath.is_absolute()
        || dirnamePath.has_parent_path()
        || dirnamePath == "."
        || dirnamePath == "..") {
        return false;
    }

    const std::filesystem::path currentDir = kconfig::data::path_resolver::UserConfigDirectory();
    const std::filesystem::path baseDir = currentDir.parent_path();
    if (baseDir.empty()) {
        return false;
    }

    const std::filesystem::path target = kconfig::data::path_utils::Canonicalize(baseDir / dirnamePath);
    kconfig::data::path_resolver::SetUserConfigRootOverride(target);
    return true;
}

bool HasUserConfigFile() {
    const std::filesystem::path filePath = CurrentUserConfigFilePath();
    KTRACE("store.requests",
           "HasUserConfigFile path='{}'",
           filePath.string());
    std::error_code ec;
    return std::filesystem::exists(filePath, ec) && !ec && std::filesystem::is_regular_file(filePath, ec) && !ec;
}

bool InitializeUserConfigFile(const kconfig::json::Value& json, std::string* error) {
    const std::filesystem::path filePath = CurrentUserConfigFilePath();
    KTRACE("store",
           "InitializeUserConfigFile requested: path='{}' (enable store.requests for details)",
           filePath.string());
    KTRACE("store.requests",
           "InitializeUserConfigFile path='{}' json_type='{}'",
           filePath.string(),
           DescribeJsonType(json));
    if (!json.is_object()) {
        if (error) {
            *error = "User config root must be a JSON object.";
        }
        return false;
    }
    return kconfig::data::path_utils::WriteJsonFile(filePath, json, error);
}

bool LoadUserConfigFile(std::string_view name,
                        const LoadUserConfigFileOptions& options,
                        std::string* error) {
    const std::filesystem::path filePath = CurrentUserConfigFilePath();
    KTRACE("store",
           "LoadUserConfigFile requested: namespace='{}' mode='{}' path='{}' (enable store.requests for details)",
           std::string(name),
           UserConfigLoadModeName(options.mode),
           filePath.string());
    KTRACE("store.requests",
           "LoadUserConfigFile namespace='{}' mode='{}' path='{}'",
           std::string(name),
           UserConfigLoadModeName(options.mode),
           filePath.string());
    if (name.empty()) {
        if (error) {
            *error = "Namespace must not be empty.";
        }
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(filePath, ec) || ec || !std::filesystem::is_regular_file(filePath, ec) || ec) {
        if (error) {
            *error = "User config file not found: " + filePath.string();
        }
        return false;
    }

    const bool ok = (options.mode == UserConfigLoadMode::Mutable)
        ? LoadMutable(name, filePath)
        : LoadReadOnly(name, filePath);
    if (!ok && error) {
        *error = "Failed to load user config file: " + filePath.string();
    }
    return ok;
}

bool SetSaveIntervalSeconds(std::string_view name, std::optional<double> seconds) {
    KTRACE("store.requests",
           "SetSaveIntervalSeconds namespace='{}' interval='{}'",
           std::string(name),
           DescribeSaveInterval(seconds));
    if (name.empty()) {
        return false;
    }
    if (seconds.has_value() && (!std::isfinite(*seconds) || *seconds < 0.0)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const std::string key(name);
    const auto configIt = internal::g_state.namedConfigs.find(key);
    if (configIt == internal::g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return false;
    }

    auto& saveState = internal::g_state.namedSaveStates[key];
    saveState.saveIntervalSeconds = seconds;
    internal::g_state.revision++;
    return true;
}

std::optional<double> SaveIntervalSeconds(std::string_view name) {
    KTRACE("store.requests",
           "SaveIntervalSeconds namespace='{}'",
           std::string(name));
    if (name.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(internal::g_state.mutex);
    const std::string key(name);
    const auto configIt = internal::g_state.namedConfigs.find(key);
    if (configIt == internal::g_state.namedConfigs.end() || !configIt->second.isMutable) {
        return std::nullopt;
    }

    const auto saveIt = internal::g_state.namedSaveStates.find(key);
    if (saveIt == internal::g_state.namedSaveStates.end()) {
        return std::nullopt;
    }
    return saveIt->second.saveIntervalSeconds;
}

bool FlushWrites(std::string* error) {
    KTRACE("store",
           "FlushWrites requested (enable store.requests for details)");
    KTRACE("store.requests",
           "FlushWrites begin");

    struct PendingWrite {
        std::string name;
        std::filesystem::path path;
        kconfig::json::Value value;
        uint64_t revision = 0;
    };
    std::vector<PendingWrite> pendingWrites;
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        for (auto& [name, config] : internal::g_state.namedConfigs) {
            if (!config.isMutable) {
                continue;
            }

            const auto saveIt = internal::g_state.namedSaveStates.find(name);
            if (saveIt == internal::g_state.namedSaveStates.end()) {
                continue;
            }
            auto& saveState = saveIt->second;
            if (saveState.pendingSave && config.revision <= saveState.lastSavedRevision) {
                saveState.pendingSave = false;
            }
            if (!saveState.pendingSave) {
                continue;
            }

            const auto backingIt = internal::g_state.namedBackingFiles.find(name);
            if (backingIt == internal::g_state.namedBackingFiles.end()) {
                KTRACE("store.requests",
                       "FlushWrites namespace='{}' pending=true backing='none' interval='{}' -> skip",
                       name,
                       DescribeSaveInterval(saveState.saveIntervalSeconds));
                continue;
            }

            const double elapsedSeconds = std::chrono::duration<double>(now - saveState.lastSaveTime).count();
            const bool due = !saveState.saveIntervalSeconds.has_value()
                || *saveState.saveIntervalSeconds <= 0.0
                || elapsedSeconds >= *saveState.saveIntervalSeconds;
            KTRACE("store.requests",
                   "FlushWrites namespace='{}' pending=true revision={} last_saved_revision={} interval='{}' elapsed={:.3f}s due={}",
                   name,
                   config.revision,
                   saveState.lastSavedRevision,
                   DescribeSaveInterval(saveState.saveIntervalSeconds),
                   elapsedSeconds,
                   due);
            if (!due) {
                continue;
            }

            pendingWrites.push_back(PendingWrite{
                name,
                backingIt->second,
                config.json,
                config.revision
            });
        }
    }

    KTRACE("store.requests",
           "FlushWrites pending_writes={}",
           pendingWrites.size());
    bool allWritesSucceeded = true;
    for (const auto& pending : pendingWrites) {
        std::string writeError;
        KTRACE("store.requests",
               "FlushWrites namespace='{}' -> WriteJsonFile",
               pending.name);
        if (!writeJsonFileUnlocked(pending.path, pending.value, &writeError)) {
            allWritesSucceeded = false;
            KTRACE("store.requests",
                   "FlushWrites namespace='{}' -> write failed: {}",
                   pending.name,
                   writeError);
            if (error && error->empty()) {
                *error = "FlushWrites failed for '" + pending.name + "': " + writeError;
            }
            continue;
        }

        const auto writeTime = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(internal::g_state.mutex);
        const auto configIt = internal::g_state.namedConfigs.find(pending.name);
        const auto saveIt = internal::g_state.namedSaveStates.find(pending.name);
        const auto backingIt = internal::g_state.namedBackingFiles.find(pending.name);
        if (configIt == internal::g_state.namedConfigs.end()
            || saveIt == internal::g_state.namedSaveStates.end()
            || backingIt == internal::g_state.namedBackingFiles.end()
            || !configIt->second.isMutable) {
            continue;
        }

        auto& saveState = saveIt->second;
        saveState.lastSaveTime = writeTime;
        saveState.lastSavedRevision = pending.revision;
        saveState.pendingSave =
            (configIt->second.revision != pending.revision) || (backingIt->second != pending.path);
        KTRACE("store.requests",
               "FlushWrites namespace='{}' -> saved revision={} pending_after={}",
               pending.name,
               pending.revision,
               saveState.pendingSave);
    }

    if (!allWritesSucceeded && error && error->empty()) {
        *error = "FlushWrites failed.";
    }
    return allWritesSucceeded;
}

} // namespace kconfig::store
