#include "network/tests/structured_log_event_sink.hpp"

namespace karma::network::tests {

size_t StructuredLogEventSink::CountLevel(spdlog::level::level_enum level) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    size_t count = 0;
    for (const auto& event : events_) {
        if (event.level == level) {
            ++count;
        }
    }
    return count;
}

size_t StructuredLogEventSink::CountLevelForLogger(spdlog::level::level_enum level,
                                                   std::string_view logger_name) {
    std::lock_guard<std::mutex> lock(this->mutex_);
    size_t count = 0;
    for (const auto& event : events_) {
        if (event.level == level && event.logger_name == logger_name) {
            ++count;
        }
    }
    return count;
}

void StructuredLogEventSink::sink_it_(const spdlog::details::log_msg& msg) {
    CapturedLogEvent event{};
    event.level = msg.level;
    event.logger_name.assign(msg.logger_name.data(), msg.logger_name.size());
    events_.push_back(std::move(event));
}

void StructuredLogEventSink::flush_() {}

} // namespace karma::network::tests
