#include "karma/common/world_archive.hpp"

#include "karma/common/logging.hpp"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <miniz.h>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace {
constexpr int kMaxArchiveEntries = 4096;
constexpr uint64_t kMaxArchiveTotalUncompressedBytes = 1024ULL * 1024ULL * 1024ULL; // 1 GiB
constexpr uint64_t kMaxArchiveFileUncompressedBytes = 256ULL * 1024ULL * 1024ULL; // 256 MiB
constexpr uint16_t kZipHostUnix = 3;
constexpr uint32_t kPosixTypeMask = 0170000u;
constexpr uint32_t kPosixTypeRegular = 0100000u;
constexpr uint32_t kPosixTypeDirectory = 0040000u;

struct PreparedArchiveEntry {
    int index = 0;
    fs::path relativePath{};
    bool isDirectory = false;
    uint64_t uncompressedSize = 0;
};

std::optional<fs::path> NormalizeArchiveEntryPath(const char* rawName) {
    if (!rawName || *rawName == '\0') {
        return std::nullopt;
    }

    std::string sanitized(rawName);
    std::replace(sanitized.begin(), sanitized.end(), '\\', '/');

    fs::path path = fs::path(sanitized).lexically_normal();
    if (path.empty() || path == ".") {
        return std::nullopt;
    }

    if (path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        return std::nullopt;
    }

    for (const auto& component : path) {
        if (component == "..") {
            return std::nullopt;
        }
    }

    return path;
}

bool BuildExtractionPlan(mz_zip_archive& zip,
                         std::vector<PreparedArchiveEntry>& entries,
                         uint64_t& totalUncompressedBytes) {
    const int numEntries = mz_zip_reader_get_num_files(&zip);
    if (numEntries < 0) {
        spdlog::error("WorldArchive: Invalid zip entry count");
        return false;
    }
    if (numEntries > kMaxArchiveEntries) {
        spdlog::error("WorldArchive: Rejecting archive with {} entries (max {})",
                      numEntries,
                      kMaxArchiveEntries);
        return false;
    }

    entries.clear();
    entries.reserve(static_cast<size_t>(numEntries));
    totalUncompressedBytes = 0;

    for (int i = 0; i < numEntries; ++i) {
        mz_zip_archive_file_stat fileStat{};
        if (!mz_zip_reader_file_stat(&zip, i, &fileStat)) {
            spdlog::error("WorldArchive: Failed to get file stat for index {}", i);
            return false;
        }

        const auto normalizedPath = NormalizeArchiveEntryPath(fileStat.m_filename);
        if (!normalizedPath.has_value()) {
            spdlog::error("WorldArchive: Rejecting unsafe archive path '{}'",
                          fileStat.m_filename ? fileStat.m_filename : "(null)");
            return false;
        }

        const bool isDirectory = mz_zip_reader_is_file_a_directory(&zip, i);
        const uint16_t hostSystem =
            static_cast<uint16_t>((static_cast<uint32_t>(fileStat.m_version_made_by) >> 8) & 0xFFu);
        const uint32_t unixMode = static_cast<uint32_t>((fileStat.m_external_attr >> 16) & 0xFFFFu);
        if (hostSystem == kZipHostUnix && (unixMode & kPosixTypeMask) != 0) {
            const uint32_t posixType = unixMode & kPosixTypeMask;
            const bool modeIsDirectory = posixType == kPosixTypeDirectory;
            const bool modeIsRegular = posixType == kPosixTypeRegular;
            const bool supportedType = isDirectory ? modeIsDirectory : modeIsRegular;
            if (!supportedType) {
                spdlog::error(
                    "WorldArchive: Rejecting unsupported archive entry type '{}' (mode={:#o}, directory={})",
                    normalizedPath->string(),
                    unixMode,
                    isDirectory ? 1 : 0);
                return false;
            }
        }

        const uint64_t uncompressedSize = static_cast<uint64_t>(fileStat.m_uncomp_size);
        if (!isDirectory && uncompressedSize > kMaxArchiveFileUncompressedBytes) {
            spdlog::error("WorldArchive: Rejecting oversized archive entry '{}' ({} bytes, max {})",
                          normalizedPath->string(),
                          uncompressedSize,
                          kMaxArchiveFileUncompressedBytes);
            return false;
        }

        if (!isDirectory) {
            if (uncompressedSize > (kMaxArchiveTotalUncompressedBytes - totalUncompressedBytes)) {
                spdlog::error("WorldArchive: Rejecting archive total size overflow at '{}' (max {} bytes)",
                              normalizedPath->string(),
                              kMaxArchiveTotalUncompressedBytes);
                return false;
            }
            totalUncompressedBytes += uncompressedSize;
        }

        entries.push_back(PreparedArchiveEntry{
            .index = i,
            .relativePath = *normalizedPath,
            .isDirectory = isDirectory,
            .uncompressedSize = uncompressedSize
        });
    }

    return true;
}

void ZipDirectory(const fs::path& inputDir, const fs::path& outputZip) {
    if (!fs::exists(inputDir) || !fs::is_directory(inputDir)) {
        throw std::runtime_error("Input is not a directory");
    }

    mz_zip_archive zip{};
    if (!mz_zip_writer_init_file(&zip, outputZip.string().c_str(), 0)) {
        throw std::runtime_error("Failed to create zip file");
    }

    try {
        for (const auto& entry : fs::recursive_directory_iterator(inputDir)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            fs::path absPath = entry.path();
            fs::path relPath = fs::relative(absPath, inputDir);
            std::string zipPath = relPath.generic_string();

            if (!mz_zip_writer_add_file(
                    &zip,
                    zipPath.c_str(),
                    absPath.string().c_str(),
                    nullptr,
                    0,
                    MZ_DEFAULT_LEVEL)) {
                throw std::runtime_error("Failed to add file: " + zipPath);
            }
        }

        if (!mz_zip_writer_finalize_archive(&zip)) {
            throw std::runtime_error("Failed to finalize zip");
        }

        mz_zip_writer_end(&zip);
    } catch (...) {
        mz_zip_writer_end(&zip);
        throw;
    }
}

world::ArchiveBytes ReadArchiveFile(const fs::path& zipPath) {
    if (!fs::exists(zipPath)) {
        throw std::runtime_error("World zip file not found: " + zipPath.string());
    }

    auto fileSize = fs::file_size(zipPath);
    world::ArchiveBytes data(fileSize);
    std::ifstream file(zipPath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open zip file: " + zipPath.string());
    }

    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    if (!file) {
        throw std::runtime_error("Failed to read zip file: " + zipPath.string());
    }

    return data;
}
}

namespace world {

ArchiveBytes BuildWorldArchive(const fs::path& worldDir) {
    const fs::path inputDir(worldDir);
    fs::path outputZip = inputDir;
    outputZip += ".zip";
    ZipDirectory(inputDir, outputZip);
    return ReadArchiveFile(outputZip);
}

bool ExtractWorldArchive(const ArchiveBytes& data, const fs::path& destDir) {
    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_mem(&zip, data.data(), data.size(), 0)) {
        spdlog::error("WorldArchive: Failed to open zip from memory");
        return false;
    }

    std::vector<PreparedArchiveEntry> entries;
    uint64_t totalUncompressedBytes = 0;
    if (!BuildExtractionPlan(zip, entries, totalUncompressedBytes)) {
        mz_zip_reader_end(&zip);
        return false;
    }

    for (const auto& entry : entries) {
        const fs::path outPath = destDir / entry.relativePath;
        std::error_code ec;
        if (entry.isDirectory) {
            fs::create_directories(outPath, ec);
            if (ec) {
                spdlog::error("WorldArchive: Failed to create directory '{}': {}",
                              outPath.string(),
                              ec.message());
                mz_zip_reader_end(&zip);
                return false;
            }
            continue;
        }

        const auto parentPath = outPath.parent_path();
        if (!parentPath.empty()) {
            fs::create_directories(parentPath, ec);
            if (ec) {
                spdlog::error("WorldArchive: Failed to create directory '{}': {}",
                              parentPath.string(),
                              ec.message());
                mz_zip_reader_end(&zip);
                return false;
            }
        }

        if (!mz_zip_reader_extract_to_file(&zip, entry.index, outPath.string().c_str(), 0)) {
            spdlog::error("WorldArchive: Failed to extract '{}'", entry.relativePath.string());
            mz_zip_reader_end(&zip);
            return false;
        }
    }

    mz_zip_reader_end(&zip);
    KARMA_TRACE("world",
                "WorldArchive: Unzipped {} entries ({} bytes) to {}",
                entries.size(),
                totalUncompressedBytes,
                destDir.string());
    return true;
}

std::optional<karma::json::Value> ReadWorldJsonFile(const fs::path& path) {
    if (!fs::exists(path)) {
        return std::nullopt;
    }

    std::ifstream file(path);
    if (!file) {
        return std::nullopt;
    }

    try {
        karma::json::Value data;
        file >> data;
        return data;
    } catch (const std::exception& e) {
        spdlog::error("WorldArchive: Failed to parse JSON {}: {}", path.string(), e.what());
        return std::nullopt;
    }
}

} // namespace world
