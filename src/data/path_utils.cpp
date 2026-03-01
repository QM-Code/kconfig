#include "data/path_utils.hpp"

#include <ktrace/trace.hpp>


#include <exception>
#include <fstream>
#include <system_error>
#include <utility>

namespace kconfig::common::data::path_utils {

std::filesystem::path Canonicalize(const std::filesystem::path &path) {
    std::error_code ec;
    auto result = std::filesystem::weakly_canonical(path, ec);
    if (!ec) {
        return result;
    }

    result = std::filesystem::absolute(path, ec);
    if (!ec) {
        return result;
    }

    return path;
}

std::filesystem::path ResolveWithBase(const std::filesystem::path &baseDir, std::string_view value) {
    std::filesystem::path candidate(value);
    if (!candidate.is_absolute()) {
        candidate = baseDir / candidate;
    }
    return Canonicalize(candidate);
}

JsonReadResult ReadJsonFile(const std::filesystem::path &path) {
    KTRACE("data", "read-json start path='{}'", path.string());
    if (!std::filesystem::exists(path)) {
        KTRACE("data", "read-json miss path='{}' reason='not found'", path.string());
        return {
            .json = std::nullopt,
            .error = JsonReadError::NotFound,
            .message = "file not found"
        };
    }

    std::ifstream stream(path);
    if (!stream) {
        KTRACE("data", "read-json failed path='{}' reason='open failed'", path.string());
        return {
            .json = std::nullopt,
            .error = JsonReadError::OpenFailed,
            .message = "failed to open file"
        };
    }

    try {
        kconfig::common::serialization::Value json;
        stream >> json;
        KTRACE("data", "read-json ok path='{}'", path.string());
        return {
            .json = std::move(json),
            .error = JsonReadError::None,
            .message = {}
        };
    } catch (const std::exception &e) {
        KTRACE("data", "read-json failed path='{}' reason='parse failed' detail='{}'",
                    path.string(),
                    e.what());
        return {
            .json = std::nullopt,
            .error = JsonReadError::ParseFailed,
            .message = e.what()
        };
    }
}

bool EnsureJsonObjectFile(const std::filesystem::path &path, std::string *error) {
    if (path.empty()) {
        KTRACE("data", "ensure-json-object failed path='<empty>' reason='empty path'");
        if (error) {
            *error = "path is empty";
        }
        return false;
    }

    KTRACE("data", "ensure-json-object start path='{}'", path.string());

    std::error_code dirEc;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, dirEc);
        if (dirEc) {
            KTRACE("data", "ensure-json-object failed path='{}' reason='create parent failed' parent='{}' detail='{}'",
                        path.string(),
                        parent.string(),
                        dirEc.message());
            if (error) {
                *error = "failed to create directory " + parent.string() + ": " + dirEc.message();
            }
            return false;
        }
    }

    const auto writeDefaultObject = [&](std::ios_base::openmode mode, const char *reason) -> bool {
        KTRACE("data", "write-json-default start path='{}' reason='{}'",
                    path.string(),
                    reason);
        std::ofstream stream(path, mode);
        if (!stream) {
            KTRACE("data", "write-json-default failed path='{}' reason='open failed'",
                        path.string());
            if (error) {
                *error = "failed to open file for writing: " + path.string();
            }
            return false;
        }
        stream << "{}\n";
        if (!stream) {
            KTRACE("data", "write-json-default failed path='{}' reason='write failed'",
                        path.string());
            if (error) {
                *error = "failed to initialize json file: " + path.string();
            }
            return false;
        }
        KTRACE("data", "write-json-default ok path='{}' content='{}'",
                    path.string(),
                    "{}");
        return true;
    };

    if (!std::filesystem::exists(path)) {
        return writeDefaultObject(std::ios::out, "missing file");
    }

    if (std::filesystem::is_regular_file(path)) {
        std::error_code sizeEc;
        const auto fileSize = std::filesystem::file_size(path, sizeEc);
        if (!sizeEc && fileSize == 0) {
            return writeDefaultObject(std::ios::out | std::ios::trunc, "empty file");
        }
    }

    KTRACE("data", "ensure-json-object no-op path='{}' reason='existing file unchanged'", path.string());
    return true;
}

bool WriteJsonFile(const std::filesystem::path &path,
                   const kconfig::common::serialization::Value &json,
                   std::string *error) {
    if (path.empty()) {
        KTRACE("data", "write-json failed path='<empty>' reason='empty path'");
        if (error) {
            *error = "path is empty";
        }
        return false;
    }

    KTRACE("data", "write-json start path='{}'", path.string());

    std::error_code dirEc;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, dirEc);
        if (dirEc) {
            KTRACE("data", "write-json failed path='{}' reason='create parent failed' parent='{}' detail='{}'",
                        path.string(),
                        parent.string(),
                        dirEc.message());
            if (error) {
                *error = "failed to create directory " + parent.string() + ": " + dirEc.message();
            }
            return false;
        }
    }

    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        KTRACE("data", "write-json failed path='{}' reason='open failed'",
                    path.string());
        if (error) {
            *error = "failed to open file for writing: " + path.string();
        }
        return false;
    }

    try {
        stream << json.dump(4) << '\n';
    } catch (const std::exception &e) {
        KTRACE("data", "write-json failed path='{}' reason='serialize/write failed' detail='{}'",
                    path.string(),
                    e.what());
        if (error) {
            *error = e.what();
        }
        return false;
    }
    KTRACE("data", "write-json ok path='{}'", path.string());
    return true;
}

void MergeJsonObjects(kconfig::common::serialization::Value &destination, const kconfig::common::serialization::Value &source) {
    if (!destination.is_object() || !source.is_object()) {
        destination = source;
        return;
    }

    for (auto it = source.begin(); it != source.end(); ++it) {
        const auto &key = it.key();
        const auto &value = it.value();
        if (value.is_object() && destination.contains(key) && destination[key].is_object()) {
            MergeJsonObjects(destination[key], value);
        } else {
            destination[key] = value;
        }
    }
}

void CollectAssetEntries(const kconfig::common::serialization::Value &node,
                         const std::filesystem::path &baseDir,
                         std::map<std::string, std::filesystem::path> &assetMap,
                         std::string_view prefix) {
    if (!node.is_object()) {
        return;
    }

    for (const auto &[key, value] : node.items()) {
        std::string fullKey;
        if (prefix.empty()) {
            fullKey = key;
        } else {
            fullKey.reserve(prefix.size() + 1 + key.size());
            fullKey.append(prefix.begin(), prefix.end());
            fullKey.push_back('.');
            fullKey.append(key);
        }

        if (value.is_string()) {
            assetMap[fullKey] = ResolveWithBase(baseDir, value.get<std::string>());
        } else if (value.is_object()) {
            CollectAssetEntries(value, baseDir, assetMap, fullKey);
        }
    }
}

} // namespace kconfig::common::data::path_utils
