#include <kconfig/i18n.hpp>
#include <spdlog/spdlog.h>

#include <kconfig/data/path_resolver.hpp>
#include <kconfig/json.hpp>
#include <kconfig/store.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

namespace kconfig::i18n {
namespace {

std::string normalizeLanguage(std::string value) {
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), value.end());
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

void flattenStrings(const kconfig::json::Value &node,
                    const std::string &prefix,
                    std::unordered_map<std::string, std::string> &out) {
    if (node.is_string()) {
        out[prefix] = node.get<std::string>();
        return;
    }
    if (!node.is_object()) {
        return;
    }
    for (auto it = node.begin(); it != node.end(); ++it) {
        const std::string key = it.key();
        if (key.empty()) {
            continue;
        }
        const std::string next = prefix.empty() ? key : (prefix + "." + key);
        flattenStrings(it.value(), next, out);
    }
}

std::unordered_map<std::string, std::string> loadLanguageStrings(const std::string &language,
                                                                 spdlog::level::level_enum missingLevel) {
    std::unordered_map<std::string, std::string> result;
    const std::filesystem::path path = kconfig::data::path_resolver::Resolve(std::filesystem::path("strings") / (language + ".json"));
    if (path.empty()) {
        spdlog::log(missingLevel, "i18n: data root not available when loading language '{}'.", language);
        return result;
    }
    const auto jsonOpt = kconfig::data::path_resolver::LoadJsonFile(path, "strings/" + language + ".json", missingLevel);
    if (!jsonOpt) {
        return result;
    }
    if (!jsonOpt->is_object()) {
        spdlog::log(missingLevel, "i18n: strings/{}.json is not a JSON object.", language);
        return result;
    }

    const auto strings_it = jsonOpt->find("strings");
    if (strings_it == jsonOpt->end() || !strings_it->is_object()) {
        spdlog::log(missingLevel, "i18n: strings/{}.json missing required top-level object 'strings'.", language);
        return result;
    }

    flattenStrings(*strings_it, "", result);
    return result;
}

} // namespace

void I18n::loadFromConfig(RuntimeRole role) {
    const char *const config_key = role == RuntimeRole::Client ? "client.Language" : "server.Language";
    std::string language = kconfig::store::ReadRequiredNonEmptyString(config_key);
    language = normalizeLanguage(language);
    if (language.empty()) {
        language = "en";
    }
    loadLanguage(language);
}

void I18n::loadLanguage(const std::string &language) {
    strings_en_ = loadLanguageStrings("en", spdlog::level::err);
    strings_selected_.clear();
    missing_cache_.clear();

    std::string normalized = normalizeLanguage(language);
    if (normalized.empty()) {
        normalized = "en";
    }
    if (normalized != "en") {
        strings_selected_ = loadLanguageStrings(normalized, spdlog::level::warn);
        if (strings_selected_.empty()) {
            spdlog::warn("i18n: falling back to English; strings/{}.json not found or empty.", normalized);
            normalized = "en";
        }
    }
    language_ = normalized;
}

const std::string &I18n::get(const std::string &key) const {
    if (auto it = strings_selected_.find(key); it != strings_selected_.end()) {
        return it->second;
    }
    if (auto it = strings_en_.find(key); it != strings_en_.end()) {
        return it->second;
    }
    auto [it, inserted] = missing_cache_.emplace(key, key);
    return it->second;
}

const std::string &I18n::require(const std::string &key) const {
    if (auto it = strings_selected_.find(key); it != strings_selected_.end()) {
        return it->second;
    }
    if (auto it = strings_en_.find(key); it != strings_en_.end()) {
        return it->second;
    }
    throw std::runtime_error("Missing required i18n key: " + key);
}

std::string I18n::format(const std::string &key,
                         std::initializer_list<std::pair<std::string, std::string>> replacements) const {
    return formatText(get(key), replacements);
}

std::string I18n::formatText(std::string_view text,
                             std::initializer_list<std::pair<std::string, std::string>> replacements) const {
    std::string result(text);
    for (const auto &pair : replacements) {
        const std::string token = "{" + pair.first + "}";
        std::size_t pos = 0;
        while ((pos = result.find(token, pos)) != std::string::npos) {
            result.replace(pos, token.size(), pair.second);
            pos += pair.second.size();
        }
    }
    return result;
}

I18n &Get() {
    static I18n instance;
    return instance;
}

} // namespace kconfig::i18n
