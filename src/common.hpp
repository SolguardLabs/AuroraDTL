#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace aurora {

class Error : public std::runtime_error {
public:
  explicit Error(const std::string &message);
};

class ValidationError : public Error {
public:
  explicit ValidationError(const std::string &message);
};

class ParseError : public Error {
public:
  explicit ParseError(const std::string &message);
};

class ExecutionError : public Error {
public:
  explicit ExecutionError(const std::string &message);
};

struct Clock {
  std::uint64_t timestamp{0};
  std::uint64_t window{0};
};

struct NamedValue {
  std::string name;
  std::string value;
};

std::string trim(const std::string &value);
std::string lower(std::string value);
std::string upper(std::string value);
std::string join(const std::vector<std::string> &items, const std::string &sep);
std::vector<std::string> split(const std::string &input, char separator);
bool startsWith(const std::string &value, const std::string &prefix);
bool endsWith(const std::string &value, const std::string &suffix);
bool isIdentifier(const std::string &value);
bool parseBoolText(const std::string &value);
std::uint64_t parseUint64(const std::string &value, const std::string &field);
std::int64_t parseInt64(const std::string &value, const std::string &field);
std::string toString(std::uint64_t value);
std::string quoted(const std::string &value);
std::string repeat(char ch, std::size_t count);
void require(bool condition, const std::string &message);

template <typename T> bool containsKey(const std::map<std::string, T> &items, const std::string &key) {
  return items.find(key) != items.end();
}

template <typename T> const T &lookup(const std::map<std::string, T> &items, const std::string &key, const std::string &kind) {
  auto it = items.find(key);
  if (it == items.end()) {
    throw ValidationError("unknown " + kind + ": " + key);
  }
  return it->second;
}

template <typename T> T &lookupMutable(std::map<std::string, T> &items, const std::string &key, const std::string &kind) {
  auto it = items.find(key);
  if (it == items.end()) {
    throw ValidationError("unknown " + kind + ": " + key);
  }
  return it->second;
}

class StringBuilder {
public:
  void append(const std::string &value);
  void append(char value);
  void appendLine(const std::string &value);
  std::string str() const;
  bool empty() const;
  std::size_t size() const;

private:
  std::string buffer_;
};

class JsonWriter {
public:
  JsonWriter();

  void beginObject();
  void endObject();
  void beginArray();
  void endArray();
  void key(const std::string &name);
  void stringValue(const std::string &value);
  void numberValue(std::uint64_t value);
  void signedNumberValue(std::int64_t value);
  void boolValue(bool value);
  void nullValue();
  void rawValue(const std::string &value);
  std::string str() const;

private:
  enum class ScopeKind { Object, Array };

  struct Scope {
    ScopeKind kind;
    bool first{true};
    bool waitingForValue{false};
  };

  void beforeValue();
  void afterValue();
  void commaIfNeeded();
  static std::string escape(const std::string &value);

  StringBuilder out_;
  std::vector<Scope> scopes_;
};

} // namespace aurora
