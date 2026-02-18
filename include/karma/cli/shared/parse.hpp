#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <ostream>
#include <string_view>
#include <string>
#include <vector>

namespace karma::cli::shared {

struct CommonState {
    bool trace_explicit = false;
    std::string trace_channels;
    bool timestamp_logging = false;
    std::string language;
    bool language_explicit = false;
    std::string data_dir;
    bool data_dir_explicit = false;
    std::string user_config_path;
    bool user_config_explicit = false;
    bool strict_config = true;
};

struct ConsumeResult {
    bool consumed = false;
    bool help_requested = false;
    std::string error;
};

enum class RegisteredOptionKind {
    Flag,
    String,
    UInt16
};

struct RegisteredOption {
    std::string short_name{};
    std::string long_name{};
    std::string value_name{};
    std::string help{};
    RegisteredOptionKind kind = RegisteredOptionKind::Flag;
    bool allow_equals_form = true;
    std::function<void()> on_flag{};
    std::function<void(const std::string&)> on_string{};
    std::function<void(uint16_t)> on_uint16{};
};

RegisteredOption DefineFlag(std::string short_name,
                            std::string long_name,
                            std::string help,
                            std::function<void()> on_flag);
RegisteredOption DefineString(std::string short_name,
                              std::string long_name,
                              std::string value_name,
                              std::string help,
                              std::function<void(const std::string&)> on_string,
                              bool allow_equals_form = true);
RegisteredOption DefineUInt16(std::string short_name,
                              std::string long_name,
                              std::string value_name,
                              std::string help,
                              std::function<void(uint16_t)> on_uint16,
                              bool allow_equals_form = true);

std::string ResolveExecutableName(const char* argv0, std::string_view fallback_name = "app");

bool StartsWith(std::string_view value, std::string_view prefix);
std::string ValueAfterEquals(const std::string& arg, std::string_view prefix);
std::optional<std::string> RequireValue(const std::string& option,
                                        int& index,
                                        int argc,
                                        char** argv,
                                        std::string* out_error);
std::optional<uint16_t> ParseUInt16Port(const std::string& value, std::string* out_error);
std::optional<bool> ParseBoolOption(const std::string& value, std::string* out_error);
std::optional<std::string> ParseChoiceLower(const std::string& raw_value,
                                            const std::vector<std::string>& allowed_values);
std::string JoinChoices(const std::vector<std::string>& values);
void RequireTraceList(int argc, char** argv);

ConsumeResult ConsumeCommonOption(const std::string& arg,
                                  int& index,
                                  int argc,
                                  char** argv,
                                  CommonState& state);
ConsumeResult ConsumePhysicsBackendOption(const std::string& arg,
                                          int& index,
                                          int argc,
                                          char** argv,
                                          std::string& backend_out,
                                          bool& explicit_out);
ConsumeResult ConsumeAudioBackendOption(const std::string& arg,
                                        int& index,
                                        int argc,
                                        char** argv,
                                        std::string& backend_out,
                                        bool& explicit_out);

bool ShouldExposePhysicsBackendOption();
bool ShouldExposeAudioBackendOption();

void AppendCommonHelp(std::ostream& out, bool include_user_config_option = true);
void AppendCoreBackendHelp(std::ostream& out);
ConsumeResult ConsumeRegisteredOption(const std::string& arg,
                                      int& index,
                                      int argc,
                                      char** argv,
                                      const std::vector<RegisteredOption>& options);
void AppendRegisteredHelp(std::ostream& out, const std::vector<RegisteredOption>& options);

} // namespace karma::cli::shared
