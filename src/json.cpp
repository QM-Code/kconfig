#include <kconfig/json.hpp>

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdint>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace kconfig::json {
namespace {

Value FromNlohmann(const nlohmann::json &json) {
    if (json.is_null()) {
        return Value(nullptr);
    }
    if (json.is_boolean()) {
        return Value(json.get<bool>());
    }
    if (json.is_number_integer()) {
        return Value(json.get<long long>());
    }
    if (json.is_number_unsigned()) {
        return Value(json.get<unsigned long long>());
    }
    if (json.is_number_float()) {
        return Value(json.get<double>());
    }
    if (json.is_string()) {
        return Value(json.get<std::string>());
    }
    if (json.is_array()) {
        Value::Array array;
        array.reserve(json.size());
        for (const auto &entry : json) {
            array.push_back(FromNlohmann(entry));
        }
        return Value(std::move(array));
    }
    if (json.is_object()) {
        Value::Object object;
        for (auto it = json.begin(); it != json.end(); ++it) {
            object.emplace(it.key(), FromNlohmann(it.value()));
        }
        return Value(std::move(object));
    }

    throw std::runtime_error("Unsupported JSON value type");
}

nlohmann::json ToNlohmann(const Value &value) {
    if (value.is_null()) {
        return nullptr;
    }
    if (value.is_boolean()) {
        return value.get<bool>();
    }
    if (value.is_number_integer()) {
        return value.get<long long>();
    }
    if (value.is_number_unsigned()) {
        return value.get<unsigned long long>();
    }
    if (value.is_number_float()) {
        return value.get<double>();
    }
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_array()) {
        nlohmann::json array = nlohmann::json::array();
        for (const auto &entry : value) {
            array.push_back(ToNlohmann(entry));
        }
        return array;
    }
    if (value.is_object()) {
        nlohmann::json object = nlohmann::json::object();
        for (auto it = value.begin(); it != value.end(); ++it) {
            object[it.key()] = ToNlohmann(it.value());
        }
        return object;
    }
    throw std::runtime_error("Unsupported JSON value type");
}

} // namespace

Value::Iterator::Iterator() = default;

Value::Iterator::Iterator(object_iterator iterator)
    : iterator_(iterator) {
}

Value::Iterator::Iterator(array_iterator iterator)
    : iterator_(iterator) {
}

Value::Iterator::reference Value::Iterator::operator*() const {
    if (const auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        return (*objectIt)->second;
    }
    if (const auto *arrayIt = std::get_if<array_iterator>(&iterator_)) {
        return **arrayIt;
    }
    throw std::logic_error("Invalid JSON iterator dereference");
}

Value::Iterator::pointer Value::Iterator::operator->() const {
    return &operator*();
}

Value::Iterator &Value::Iterator::operator++() {
    if (auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        ++(*objectIt);
    } else if (auto *arrayIt = std::get_if<array_iterator>(&iterator_)) {
        ++(*arrayIt);
    }
    return *this;
}

Value::Iterator Value::Iterator::operator++(int) {
    Iterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::Iterator::operator==(const Iterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::Iterator::operator!=(const Iterator &other) const {
    return !(*this == other);
}

const std::string &Value::Iterator::key() const {
    if (const auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        return (*objectIt)->first;
    }
    throw std::logic_error("JSON key() is only valid for object iterators");
}

Value::Iterator::reference Value::Iterator::value() const {
    return operator*();
}

Value::ConstIterator::ConstIterator() = default;

Value::ConstIterator::ConstIterator(const_object_iterator iterator)
    : iterator_(iterator) {
}

Value::ConstIterator::ConstIterator(const_array_iterator iterator)
    : iterator_(iterator) {
}

Value::ConstIterator::reference Value::ConstIterator::operator*() const {
    if (const auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        return (*objectIt)->second;
    }
    if (const auto *arrayIt = std::get_if<const_array_iterator>(&iterator_)) {
        return **arrayIt;
    }
    throw std::logic_error("Invalid JSON iterator dereference");
}

Value::ConstIterator::pointer Value::ConstIterator::operator->() const {
    return &operator*();
}

Value::ConstIterator &Value::ConstIterator::operator++() {
    if (auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        ++(*objectIt);
    } else if (auto *arrayIt = std::get_if<const_array_iterator>(&iterator_)) {
        ++(*arrayIt);
    }
    return *this;
}

Value::ConstIterator Value::ConstIterator::operator++(int) {
    ConstIterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::ConstIterator::operator==(const ConstIterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::ConstIterator::operator!=(const ConstIterator &other) const {
    return !(*this == other);
}

const std::string &Value::ConstIterator::key() const {
    if (const auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        return (*objectIt)->first;
    }
    throw std::logic_error("JSON key() is only valid for object iterators");
}

Value::ConstIterator::reference Value::ConstIterator::value() const {
    return operator*();
}

Value::ItemsIterator::ItemsIterator() = default;

Value::ItemsIterator::ItemsIterator(object_iterator iterator)
    : iterator_(iterator) {
}

Value::ItemsIterator::value_type Value::ItemsIterator::operator*() const {
    if (const auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        return {(*objectIt)->first, (*objectIt)->second};
    }
    throw std::logic_error("Invalid JSON items() iterator dereference");
}

Value::ItemsIterator &Value::ItemsIterator::operator++() {
    if (auto *objectIt = std::get_if<object_iterator>(&iterator_)) {
        ++(*objectIt);
    }
    return *this;
}

Value::ItemsIterator Value::ItemsIterator::operator++(int) {
    ItemsIterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::ItemsIterator::operator==(const ItemsIterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::ItemsIterator::operator!=(const ItemsIterator &other) const {
    return !(*this == other);
}

Value::ConstItemsIterator::ConstItemsIterator() = default;

Value::ConstItemsIterator::ConstItemsIterator(const_object_iterator iterator)
    : iterator_(iterator) {
}

Value::ConstItemsIterator::value_type Value::ConstItemsIterator::operator*() const {
    if (const auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        return {(*objectIt)->first, (*objectIt)->second};
    }
    throw std::logic_error("Invalid JSON items() iterator dereference");
}

Value::ConstItemsIterator &Value::ConstItemsIterator::operator++() {
    if (auto *objectIt = std::get_if<const_object_iterator>(&iterator_)) {
        ++(*objectIt);
    }
    return *this;
}

Value::ConstItemsIterator Value::ConstItemsIterator::operator++(int) {
    ConstItemsIterator copy = *this;
    ++(*this);
    return copy;
}

bool Value::ConstItemsIterator::operator==(const ConstItemsIterator &other) const {
    return iterator_ == other.iterator_;
}

bool Value::ConstItemsIterator::operator!=(const ConstItemsIterator &other) const {
    return !(*this == other);
}

Value::ItemsView::ItemsView()
    : object_(nullptr) {
}

Value::ItemsView::ItemsView(Object *object)
    : object_(object) {
}

Value::ItemsIterator Value::ItemsView::begin() {
    if (!object_) {
        return ItemsIterator();
    }
    return ItemsIterator(object_->begin());
}

Value::ItemsIterator Value::ItemsView::end() {
    if (!object_) {
        return ItemsIterator();
    }
    return ItemsIterator(object_->end());
}

Value::ConstItemsView::ConstItemsView()
    : object_(nullptr) {
}

Value::ConstItemsView::ConstItemsView(const Object *object)
    : object_(object) {
}

Value::ConstItemsIterator Value::ConstItemsView::begin() const {
    if (!object_) {
        return ConstItemsIterator();
    }
    return ConstItemsIterator(object_->begin());
}

Value::ConstItemsIterator Value::ConstItemsView::end() const {
    if (!object_) {
        return ConstItemsIterator();
    }
    return ConstItemsIterator(object_->end());
}

Value::Value()
    : data_(nullptr) {
}

Value::Value(std::nullptr_t)
    : data_(nullptr) {
}

Value::Value(bool value)
    : data_(value) {
}

Value::Value(int value)
    : data_(static_cast<long long>(value)) {
}

Value::Value(long long value)
    : data_(value) {
}

Value::Value(unsigned int value)
    : data_(static_cast<unsigned long long>(value)) {
}

Value::Value(unsigned long long value)
    : data_(value) {
}

Value::Value(double value)
    : data_(value) {
}

Value::Value(const char *value)
    : data_(value ? std::string(value) : std::string()) {
}

Value::Value(std::string value)
    : data_(std::move(value)) {
}

Value::Value(const Object &value)
    : data_(value) {
}

Value::Value(Object &&value)
    : data_(std::move(value)) {
}

Value::Value(const Array &value)
    : data_(value) {
}

Value::Value(Array &&value)
    : data_(std::move(value)) {
}

Value::Value(const Value &other) = default;

Value::Value(Value &&other) noexcept = default;

Value &Value::operator=(const Value &other) = default;

Value &Value::operator=(Value &&other) noexcept = default;

Value::~Value() = default;

Value &Value::operator=(std::nullptr_t) {
    data_ = nullptr;
    return *this;
}

Value &Value::operator=(bool value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(int value) {
    data_ = static_cast<long long>(value);
    return *this;
}

Value &Value::operator=(long long value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(unsigned int value) {
    data_ = static_cast<unsigned long long>(value);
    return *this;
}

Value &Value::operator=(unsigned long long value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(double value) {
    data_ = value;
    return *this;
}

Value &Value::operator=(const char *value) {
    data_ = value ? std::string(value) : std::string();
    return *this;
}

Value &Value::operator=(std::string value) {
    data_ = std::move(value);
    return *this;
}

Value Value::parse(std::string_view text) {
    const auto parsed = nlohmann::json::parse(text.begin(), text.end());
    return FromNlohmann(parsed);
}

Value Value::object() {
    return Value(Object{});
}

Value Value::array() {
    return Value(Array{});
}

bool Value::is_null() const {
    return std::holds_alternative<std::nullptr_t>(data_);
}

bool Value::is_object() const {
    return std::holds_alternative<Object>(data_);
}

bool Value::is_array() const {
    return std::holds_alternative<Array>(data_);
}

bool Value::is_string() const {
    return std::holds_alternative<std::string>(data_);
}

bool Value::is_boolean() const {
    return std::holds_alternative<bool>(data_);
}

bool Value::is_number() const {
    return std::holds_alternative<long long>(data_)
        || std::holds_alternative<unsigned long long>(data_)
        || std::holds_alternative<double>(data_);
}

bool Value::is_number_integer() const {
    return std::holds_alternative<long long>(data_)
        || std::holds_alternative<unsigned long long>(data_);
}

bool Value::is_number_unsigned() const {
    return std::holds_alternative<unsigned long long>(data_);
}

bool Value::is_number_float() const {
    return std::holds_alternative<double>(data_);
}

std::size_t Value::size() const {
    if (const auto *object = std::get_if<Object>(&data_)) {
        return object->size();
    }
    if (const auto *array = std::get_if<Array>(&data_)) {
        return array->size();
    }
    if (std::holds_alternative<std::nullptr_t>(data_)) {
        return 0;
    }
    return 1;
}

bool Value::empty() const {
    if (const auto *object = std::get_if<Object>(&data_)) {
        return object->empty();
    }
    if (const auto *array = std::get_if<Array>(&data_)) {
        return array->empty();
    }
    if (const auto *string = std::get_if<std::string>(&data_)) {
        return string->empty();
    }
    return std::holds_alternative<std::nullptr_t>(data_);
}

bool Value::contains(std::string_view key) const {
    const auto *object = std::get_if<Object>(&data_);
    if (!object) {
        return false;
    }
    return object->find(std::string(key)) != object->end();
}

Value &Value::operator[](std::string_view key) {
    if (!is_object()) {
        data_ = Object{};
    }
    auto &object = std::get<Object>(data_);
    return object[std::string(key)];
}

const Value &Value::operator[](std::string_view key) const {
    const auto *object = std::get_if<Object>(&data_);
    if (!object) {
        throw std::out_of_range("JSON value is not an object");
    }
    const auto it = object->find(std::string(key));
    if (it == object->end()) {
        throw std::out_of_range("JSON object key not found");
    }
    return it->second;
}

Value &Value::operator[](std::size_t index) {
    if (!is_array()) {
        data_ = Array{};
    }
    auto &array = std::get<Array>(data_);
    if (index >= array.size()) {
        array.resize(index + 1);
    }
    return array[index];
}

const Value &Value::operator[](std::size_t index) const {
    const auto *array = std::get_if<Array>(&data_);
    if (!array) {
        throw std::out_of_range("JSON value is not an array");
    }
    if (index >= array->size()) {
        throw std::out_of_range("JSON array index out of range");
    }
    return (*array)[index];
}

Value::Iterator Value::find(std::string_view key) {
    if (!is_object()) {
        return end();
    }
    auto &object = std::get<Object>(data_);
    return Iterator(object.find(std::string(key)));
}

Value::ConstIterator Value::find(std::string_view key) const {
    if (!is_object()) {
        return end();
    }
    const auto &object = std::get<Object>(data_);
    return ConstIterator(object.find(std::string(key)));
}

void Value::erase(std::string_view key) {
    if (!is_object()) {
        return;
    }
    auto &object = std::get<Object>(data_);
    object.erase(std::string(key));
}

void Value::push_back(const Value &value) {
    if (!is_array()) {
        data_ = Array{};
    }
    auto &array = std::get<Array>(data_);
    array.push_back(value);
}

void Value::push_back(Value &&value) {
    if (!is_array()) {
        data_ = Array{};
    }
    auto &array = std::get<Array>(data_);
    array.push_back(std::move(value));
}

void Value::push_back(std::nullptr_t) {
    push_back(Value(nullptr));
}

Value::Iterator Value::begin() {
    if (is_object()) {
        return Iterator(std::get<Object>(data_).begin());
    }
    if (is_array()) {
        return Iterator(std::get<Array>(data_).begin());
    }
    return Iterator();
}

Value::Iterator Value::end() {
    if (is_object()) {
        return Iterator(std::get<Object>(data_).end());
    }
    if (is_array()) {
        return Iterator(std::get<Array>(data_).end());
    }
    return Iterator();
}

Value::ConstIterator Value::begin() const {
    if (is_object()) {
        return ConstIterator(std::get<Object>(data_).begin());
    }
    if (is_array()) {
        return ConstIterator(std::get<Array>(data_).begin());
    }
    return ConstIterator();
}

Value::ConstIterator Value::end() const {
    if (is_object()) {
        return ConstIterator(std::get<Object>(data_).end());
    }
    if (is_array()) {
        return ConstIterator(std::get<Array>(data_).end());
    }
    return ConstIterator();
}

Value::ConstIterator Value::cbegin() const {
    return begin();
}

Value::ConstIterator Value::cend() const {
    return end();
}

Value::ItemsView Value::items() {
    if (!is_object()) {
        return ItemsView(nullptr);
    }
    return ItemsView(&std::get<Object>(data_));
}

Value::ConstItemsView Value::items() const {
    if (!is_object()) {
        return ConstItemsView(nullptr);
    }
    return ConstItemsView(&std::get<Object>(data_));
}

std::string Value::dump(int indent) const {
    return ToNlohmann(*this).dump(indent);
}

template <>
bool Value::get<bool>() const {
    if (const auto *boolean = std::get_if<bool>(&data_)) {
        return *boolean;
    }
    throw std::runtime_error("JSON value is not a boolean");
}

template <>
long long Value::get<long long>() const {
    if (const auto *signedNumber = std::get_if<long long>(&data_)) {
        return *signedNumber;
    }
    if (const auto *unsignedNumber = std::get_if<unsigned long long>(&data_)) {
        if (*unsignedNumber <= static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
            return static_cast<long long>(*unsignedNumber);
        }
        throw std::runtime_error("JSON unsigned number cannot fit in long long");
    }
    if (const auto *floating = std::get_if<double>(&data_)) {
        if (std::isfinite(*floating)
            && std::trunc(*floating) == *floating
            && *floating >= static_cast<double>(std::numeric_limits<long long>::min())
            && *floating <= static_cast<double>(std::numeric_limits<long long>::max())) {
            return static_cast<long long>(*floating);
        }
        throw std::runtime_error("JSON floating number cannot fit in long long");
    }
    throw std::runtime_error("JSON value is not an integer number");
}

template <>
unsigned long long Value::get<unsigned long long>() const {
    if (const auto *unsignedNumber = std::get_if<unsigned long long>(&data_)) {
        return *unsignedNumber;
    }
    if (const auto *signedNumber = std::get_if<long long>(&data_)) {
        if (*signedNumber >= 0) {
            return static_cast<unsigned long long>(*signedNumber);
        }
        throw std::runtime_error("Negative JSON number cannot fit in unsigned long long");
    }
    if (const auto *floating = std::get_if<double>(&data_)) {
        if (std::isfinite(*floating)
            && std::trunc(*floating) == *floating
            && *floating >= 0.0
            && *floating <= static_cast<double>(std::numeric_limits<unsigned long long>::max())) {
            return static_cast<unsigned long long>(*floating);
        }
        throw std::runtime_error("JSON floating number cannot fit in unsigned long long");
    }
    throw std::runtime_error("JSON value is not an unsigned integer number");
}

template <>
double Value::get<double>() const {
    if (const auto *floating = std::get_if<double>(&data_)) {
        return *floating;
    }
    if (const auto *signedNumber = std::get_if<long long>(&data_)) {
        return static_cast<double>(*signedNumber);
    }
    if (const auto *unsignedNumber = std::get_if<unsigned long long>(&data_)) {
        return static_cast<double>(*unsignedNumber);
    }
    throw std::runtime_error("JSON value is not a numeric value");
}

template <>
std::string Value::get<std::string>() const {
    if (const auto *string = std::get_if<std::string>(&data_)) {
        return *string;
    }
    throw std::runtime_error("JSON value is not a string");
}

template <>
int Value::get<int>() const {
    const long long value = get<long long>();
    if (value < static_cast<long long>(std::numeric_limits<int>::min())
        || value > static_cast<long long>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("JSON number cannot fit in int");
    }
    return static_cast<int>(value);
}

template <>
unsigned int Value::get<unsigned int>() const {
    const unsigned long long value = get<unsigned long long>();
    if (value > static_cast<unsigned long long>(std::numeric_limits<unsigned int>::max())) {
        throw std::runtime_error("JSON number cannot fit in unsigned int");
    }
    return static_cast<unsigned int>(value);
}

template <>
float Value::get<float>() const {
    const double value = get<double>();
    if (!std::isfinite(value)
        || value < static_cast<double>(-std::numeric_limits<float>::max())
        || value > static_cast<double>(std::numeric_limits<float>::max())) {
        throw std::runtime_error("JSON number cannot fit in float");
    }
    return static_cast<float>(value);
}

Value Parse(std::string_view text) {
    return Value::parse(text);
}

Value Object() {
    return Value::object();
}

Value Array() {
    return Value::array();
}

std::string Dump(const Value &value, int indent) {
    return value.dump(indent);
}

std::istream &operator>>(std::istream &input, Value &value) {
    nlohmann::json json;
    input >> json;
    value = FromNlohmann(json);
    return input;
}

std::ostream &operator<<(std::ostream &output, const Value &value) {
    output << value.dump();
    return output;
}

} // namespace kconfig::json
