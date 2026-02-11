#pragma once

#include <spdlog/sinks/base_sink.h>

#include <cstddef>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace karma::network::tests {

struct CapturedLogEvent {
    spdlog::level::level_enum level = spdlog::level::off;
    std::string logger_name{};
};

class StructuredLogEventSink final : public spdlog::sinks::base_sink<std::mutex> {
 public:
    size_t CountLevel(spdlog::level::level_enum level);
    size_t CountLevelForLogger(spdlog::level::level_enum level, std::string_view logger_name);

 protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override;

 private:
    std::vector<CapturedLogEvent> events_{};
};

} // namespace karma::network::tests
