#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace karma::network::content {

void HashChunkChainFNV1a(uint64_t& hash, uint32_t chunk_index, const std::vector<std::byte>& chunk_data);
bool IsChunkInTransferBounds(uint64_t total_bytes,
                             uint32_t chunk_size,
                             uint32_t chunk_index,
                             size_t chunk_bytes);
bool ChunkMatchesBufferedPayload(const std::vector<std::byte>& payload,
                                 size_t chunk_offset,
                                 const std::vector<std::byte>& chunk_data);

} // namespace karma::network::content
