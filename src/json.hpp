#pragma once

#include "common.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

class JsonValue {
public:
  enum class Type { Null, Bool, Number, String, Array, Object };

  JsonValue();
  explicit JsonValue(std::nullptr_t);
  explicit JsonValue(bool value);
  explicit JsonValue(std::string value, bool numberToken = false);
  static JsonValue array(std::vector<JsonValue> values);
  static JsonValue object(std::map<std::string, JsonValue> values);

  Type type() const;
  bool isNull() const;
  bool isBool() const;
  bool isNumber() const;
  bool isString() const;
  bool isArray() const;
  bool isObject() const;

  bool asBool(const std::string &field) const;
  const std::string &asString(const std::string &field) const;
  const std::string &asNumberToken(const std::string &field) const;
  std::uint64_t asUint64(const std::string &field) const;
  std::int64_t asInt64(const std::string &field) const;
  const std::vector<JsonValue> &asArray(const std::string &field) const;
  const std::map<std::string, JsonValue> &asObject(const std::string &field) const;

  bool has(const std::string &key) const;
  const JsonValue &at(const std::string &key) const;
  const JsonValue *maybe(const std::string &key) const;

private:
  Type type_{Type::Null};
  bool boolValue_{false};
  std::string text_;
  std::vector<JsonValue> array_;
  std::map<std::string, JsonValue> object_;
};

JsonValue parseJsonText(const std::string &text);
JsonValue parseJsonFile(const std::string &path);

std::string jsonRequiredString(const JsonValue &object, const std::string &field);
std::string jsonOptionalString(const JsonValue &object, const std::string &field, const std::string &fallback);
std::uint64_t jsonRequiredUint64(const JsonValue &object, const std::string &field);
std::uint64_t jsonOptionalUint64(const JsonValue &object, const std::string &field, std::uint64_t fallback);
bool jsonOptionalBool(const JsonValue &object, const std::string &field, bool fallback);
std::vector<std::string> jsonStringArray(const JsonValue &object, const std::string &field);

} // namespace aurora
