#include "server/domain/world_session.hpp"

#include "karma/common/config_helpers.hpp"
#include "karma/common/config_store.hpp"
#include "karma/common/data_path_resolver.hpp"
#include "karma/common/logging.hpp"
#include "karma/common/world_archive.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

namespace bz3::server::domain {

namespace {

std::string ResolveWorldDirectory(const CLIOptions& options) {
    if (options.world_specified) {
        return options.world_dir;
    }
    return karma::config::ReadStringConfig("defaultWorld", "");
}

void HashBytesFNV1a(uint64_t& hash, const std::byte* bytes, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        hash ^= static_cast<uint64_t>(std::to_integer<unsigned char>(bytes[i]));
        hash *= 1099511628211ULL;
    }
}

void HashStringFNV1a(uint64_t& hash, std::string_view value) {
    const auto* bytes = reinterpret_cast<const std::byte*>(value.data());
    HashBytesFNV1a(hash, bytes, value.size());
}

std::string Hash64ToHex(uint64_t hash) {
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

std::string ComputeWorldPackageHash(const std::vector<std::byte>& bytes) {
    // FNV-1a 64-bit hash for package-byte identity.
    uint64_t hash = 14695981039346656037ULL;
    for (const std::byte value : bytes) {
        hash ^= static_cast<uint64_t>(std::to_integer<unsigned char>(value));
        hash *= 1099511628211ULL;
    }

    return Hash64ToHex(hash);
}

struct WorldManifestSummary {
    std::string content_hash{};
    std::string manifest_hash{};
    std::vector<WorldManifestEntry> entries{};
};

WorldManifestSummary ComputeWorldManifestSummary(const std::filesystem::path& world_dir) {
    std::vector<std::filesystem::path> files{};
    for (const auto& entry : std::filesystem::recursive_directory_iterator(world_dir)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        files.push_back(entry.path());
    }

    std::sort(files.begin(), files.end());

    uint64_t content_hash = 14695981039346656037ULL;
    uint64_t manifest_hash = 14695981039346656037ULL;
    std::array<char, 64 * 1024> buffer{};
    const std::byte separator = std::byte{0};
    std::vector<WorldManifestEntry> entries{};
    entries.reserve(files.size());
    for (const auto& file_path : files) {
        const std::filesystem::path rel_path = std::filesystem::relative(file_path, world_dir);
        const std::string rel = rel_path.generic_string();
        HashStringFNV1a(content_hash, rel);
        HashBytesFNV1a(content_hash, &separator, 1);

        std::ifstream input(file_path, std::ios::binary);
        if (!input) {
            throw std::runtime_error("Failed to open world file for content hash: " + file_path.string());
        }

        uint64_t file_hash = 14695981039346656037ULL;
        uint64_t file_size = 0;
        while (input.good()) {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize read_count = input.gcount();
            if (read_count > 0) {
                const auto* bytes = reinterpret_cast<const std::byte*>(buffer.data());
                HashBytesFNV1a(content_hash, bytes, static_cast<size_t>(read_count));
                HashBytesFNV1a(file_hash, bytes, static_cast<size_t>(read_count));
                file_size += static_cast<uint64_t>(read_count);
            }
        }
        if (!input.eof()) {
            throw std::runtime_error("Failed while hashing world file: " + file_path.string());
        }
        HashBytesFNV1a(content_hash, &separator, 1);

        const std::string file_hash_hex = Hash64ToHex(file_hash);
        const std::string file_size_text = std::to_string(file_size);
        HashStringFNV1a(manifest_hash, rel);
        HashBytesFNV1a(manifest_hash, &separator, 1);
        HashStringFNV1a(manifest_hash, file_size_text);
        HashBytesFNV1a(manifest_hash, &separator, 1);
        HashStringFNV1a(manifest_hash, file_hash_hex);
        HashBytesFNV1a(manifest_hash, &separator, 1);
        entries.push_back(WorldManifestEntry{
            .path = rel,
            .size = file_size,
            .hash = file_hash_hex});
    }

    return WorldManifestSummary{
        .content_hash = Hash64ToHex(content_hash),
        .manifest_hash = Hash64ToHex(manifest_hash),
        .entries = std::move(entries)};
}

} // namespace

std::optional<WorldSessionContext> LoadWorldSessionContext(const CLIOptions& options) {
    const std::string world_dir = ResolveWorldDirectory(options);
    if (world_dir.empty()) {
        spdlog::error("bz3-server: no world directory specified and 'defaultWorld' is missing.");
        return std::nullopt;
    }

    const std::filesystem::path world_dir_path = karma::data::Resolve(world_dir);
    if (!std::filesystem::is_directory(world_dir_path)) {
        spdlog::error("bz3-server: world directory not found: {}", world_dir_path.string());
        return std::nullopt;
    }

    const std::filesystem::path world_config_path = world_dir_path / "config.json";
    const auto world_config_opt =
        karma::data::LoadJsonFile(world_config_path, "world config", spdlog::level::err);
    if (!world_config_opt || !world_config_opt->is_object()) {
        spdlog::error("bz3-server: failed to load world config object from {}", world_config_path.string());
        return std::nullopt;
    }

    if (!karma::config::ConfigStore::AddRuntimeLayer("world config", *world_config_opt, world_config_path.parent_path())) {
        spdlog::error("bz3-server: failed to add world config runtime layer.");
        return std::nullopt;
    }

    const auto* layer = karma::config::ConfigStore::LayerByLabel("world config");
    if (!layer || !layer->is_object()) {
        spdlog::error("bz3-server: runtime layer lookup failed for world config.");
        return std::nullopt;
    }

    WorldSessionContext context{};
    context.world_dir = world_dir_path;
    context.world_config_path = world_config_path;
    context.world_name =
        karma::config::ReadStringConfig("worldName", world_dir_path.filename().string());
    context.world_package_enabled = options.world_specified;

    if (context.world_package_enabled) {
        try {
            context.world_package = world::BuildWorldArchive(context.world_dir);
        } catch (const std::exception& ex) {
            spdlog::error("bz3-server: failed to package world '{}': {}",
                          context.world_dir.string(),
                          ex.what());
            return std::nullopt;
        }

        context.world_package_size = static_cast<uint64_t>(context.world_package.size());
        context.world_package_hash = ComputeWorldPackageHash(context.world_package);
        const WorldManifestSummary manifest = ComputeWorldManifestSummary(context.world_dir);
        context.world_content_hash = manifest.content_hash;
        context.world_manifest_hash = manifest.manifest_hash;
        context.world_manifest = manifest.entries;
        context.world_manifest_file_count = static_cast<uint32_t>(context.world_manifest.size());

        KARMA_TRACE("net.server",
                    "bz3-server: packaged custom world '{}' bytes={} hash={} content_hash={} manifest_hash={} files={} dir='{}'",
                    context.world_name,
                    context.world_package_size,
                    context.world_package_hash,
                    context.world_content_hash.empty() ? "-" : context.world_content_hash,
                    context.world_manifest_hash.empty() ? "-" : context.world_manifest_hash,
                    context.world_manifest_file_count,
                    context.world_dir.string());
    } else {
        KARMA_TRACE("net.server",
                    "bz3-server: bundled world mode '{}' (no world package transfer)",
                    context.world_name);
    }

    context.world_id = karma::config::ReadStringConfig("worldId", context.world_name);
    if (context.world_package_enabled) {
        const std::string default_revision = !context.world_content_hash.empty()
            ? context.world_content_hash
            : (context.world_package_hash.empty() ? std::string("custom") : context.world_package_hash);
        context.world_revision = karma::config::ReadStringConfig("worldRevision",
                                                                 default_revision);
    } else {
        context.world_revision = karma::config::ReadStringConfig("worldRevision", "bundled");
    }

    KARMA_TRACE("engine.server",
                "bz3-server: world '{}' (id='{}' rev='{}') loaded from '{}'",
                context.world_name,
                context.world_id,
                context.world_revision,
                context.world_dir.string());
    return context;
}

} // namespace bz3::server::domain
