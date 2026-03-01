#pragma once

#include <initializer_list>
#include <string>
#include <string_view>
#include <unordered_map>

namespace kconfig::common::i18n {

class I18n {
public:
    enum class RuntimeRole {
        Client,
        Server
    };

    void loadFromConfig(RuntimeRole role);
    const std::string &get(const std::string &key) const;
    const std::string &require(const std::string &key) const;
    std::string formatText(std::string_view text,
                           std::initializer_list<std::pair<std::string, std::string>> replacements) const;

private:
    void loadLanguage(const std::string &language);
    std::string format(const std::string &key,
                       std::initializer_list<std::pair<std::string, std::string>> replacements) const;

    std::string language_ = "en";
    std::unordered_map<std::string, std::string> strings_en_;
    std::unordered_map<std::string, std::string> strings_selected_;
    mutable std::unordered_map<std::string, std::string> missing_cache_;
};

I18n &Get();

} // namespace kconfig::common::i18n
