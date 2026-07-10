#include "common.hpp"

#include <limits>

namespace aurora {

Error::Error(const std::string &message) : std::runtime_error(message) {}

ValidationError::ValidationError(const std::string &message) : Error(message) {}

ParseError::ParseError(const std::string &message) : Error(message) {}

ExecutionError::ExecutionError(const std::string &message) : Error(message) {}

std::string trim(const std::string &value) {
  std::size_t first = 0;
  while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first])) != 0) {
    ++first;
  }
  std::size_t last = value.size();
  while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
    --last;
  }
  return value.substr(first, last - first);
}

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string upper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return value;
}

std::string join(const std::vector<std::string> &items, const std::string &sep) {
  std::ostringstream out;
  for (std::size_t i = 0; i < items.size(); ++i) {
    if (i != 0) {
      out << sep;
    }
    out << items[i];
  }
  return out.str();
}

std::vector<std::string> split(const std::string &input, char separator) {
  std::vector<std::string> items;
  std::string current;
  for (char ch : input) {
    if (ch == separator) {
      items.push_back(current);
      current.clear();
    } else {
      current.push_back(ch);
    }
  }
  items.push_back(current);
  return items;
}

bool startsWith(const std::string &value, const std::string &prefix) {
  if (prefix.size() > value.size()) {
    return false;
  }
  return std::equal(prefix.begin(), prefix.end(), value.begin());
}

bool endsWith(const std::string &value, const std::string &suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

bool isIdentifier(const std::string &value) {
  if (value.empty()) {
    return false;
  }
  for (char ch : value) {
    const bool ok = std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '-' || ch == '.';
    if (!ok) {
      return false;
    }
  }
  return true;
}

bool parseBoolText(const std::string &value) {
  const std::string normalized = lower(trim(value));
  if (normalized == "true" || normalized == "1" || normalized == "yes") {
    return true;
  }
  if (normalized == "false" || normalized == "0" || normalized == "no") {
    return false;
  }
  throw ParseError("invalid boolean text: " + value);
}

std::uint64_t parseUint64(const std::string &value, const std::string &field) {
  const std::string text = trim(value);
  if (text.empty()) {
    throw ParseError("empty unsigned integer for " + field);
  }
  std::uint64_t result = 0;
  for (char ch : text) {
    if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
      throw ParseError("invalid unsigned integer for " + field + ": " + value);
    }
    const std::uint64_t digit = static_cast<std::uint64_t>(ch - '0');
    if (result > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
      throw ParseError("unsigned integer overflow for " + field + ": " + value);
    }
    result = (result * 10U) + digit;
  }
  return result;
}

std::int64_t parseInt64(const std::string &value, const std::string &field) {
  const std::string text = trim(value);
  if (text.empty()) {
    throw ParseError("empty integer for " + field);
  }
  bool negative = false;
  std::size_t offset = 0;
  if (text[0] == '-') {
    negative = true;
    offset = 1;
  }
  if (offset == text.size()) {
    throw ParseError("invalid integer for " + field + ": " + value);
  }
  std::uint64_t magnitude = 0;
  for (std::size_t i = offset; i < text.size(); ++i) {
    const char ch = text[i];
    if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
      throw ParseError("invalid integer for " + field + ": " + value);
    }
    const std::uint64_t digit = static_cast<std::uint64_t>(ch - '0');
    if (magnitude > (static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1U - digit) / 10U) {
      throw ParseError("integer overflow for " + field + ": " + value);
    }
    magnitude = (magnitude * 10U) + digit;
  }
  if (negative) {
    const std::uint64_t limit = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1U;
    if (magnitude == limit) {
      return std::numeric_limits<std::int64_t>::min();
    }
    return -static_cast<std::int64_t>(magnitude);
  }
  if (magnitude > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
    throw ParseError("integer overflow for " + field + ": " + value);
  }
  return static_cast<std::int64_t>(magnitude);
}

std::string toString(std::uint64_t value) {
  return std::to_string(value);
}

std::string quoted(const std::string &value) {
  return "\"" + value + "\"";
}

std::string repeat(char ch, std::size_t count) {
  return std::string(count, ch);
}

void require(bool condition, const std::string &message) {
  if (!condition) {
    throw ValidationError(message);
  }
}

void StringBuilder::append(const std::string &value) {
  buffer_ += value;
}

void StringBuilder::append(char value) {
  buffer_.push_back(value);
}

void StringBuilder::appendLine(const std::string &value) {
  buffer_ += value;
  buffer_.push_back('\n');
}

std::string StringBuilder::str() const {
  return buffer_;
}

bool StringBuilder::empty() const {
  return buffer_.empty();
}

std::size_t StringBuilder::size() const {
  return buffer_.size();
}

JsonWriter::JsonWriter() = default;

void JsonWriter::beginObject() {
  beforeValue();
  out_.append('{');
  scopes_.push_back(Scope{ScopeKind::Object, true, false});
}

void JsonWriter::endObject() {
  if (scopes_.empty() || scopes_.back().kind != ScopeKind::Object) {
    throw ExecutionError("json writer object scope mismatch");
  }
  if (scopes_.back().waitingForValue) {
    throw ExecutionError("json writer object key without value");
  }
  scopes_.pop_back();
  out_.append('}');
  afterValue();
}

void JsonWriter::beginArray() {
  beforeValue();
  out_.append('[');
  scopes_.push_back(Scope{ScopeKind::Array, true, false});
}

void JsonWriter::endArray() {
  if (scopes_.empty() || scopes_.back().kind != ScopeKind::Array) {
    throw ExecutionError("json writer array scope mismatch");
  }
  scopes_.pop_back();
  out_.append(']');
  afterValue();
}

void JsonWriter::key(const std::string &name) {
  if (scopes_.empty() || scopes_.back().kind != ScopeKind::Object) {
    throw ExecutionError("json writer key outside object");
  }
  Scope &scope = scopes_.back();
  if (scope.waitingForValue) {
    throw ExecutionError("json writer key before previous value");
  }
  commaIfNeeded();
  out_.append(escape(name));
  out_.append(':');
  scope.waitingForValue = true;
}

void JsonWriter::stringValue(const std::string &value) {
  beforeValue();
  out_.append(escape(value));
  afterValue();
}

void JsonWriter::numberValue(std::uint64_t value) {
  beforeValue();
  out_.append(std::to_string(value));
  afterValue();
}

void JsonWriter::signedNumberValue(std::int64_t value) {
  beforeValue();
  out_.append(std::to_string(value));
  afterValue();
}

void JsonWriter::boolValue(bool value) {
  beforeValue();
  out_.append(value ? "true" : "false");
  afterValue();
}

void JsonWriter::nullValue() {
  beforeValue();
  out_.append("null");
  afterValue();
}

void JsonWriter::rawValue(const std::string &value) {
  beforeValue();
  out_.append(value);
  afterValue();
}

std::string JsonWriter::str() const {
  if (!scopes_.empty()) {
    throw ExecutionError("json writer has unclosed scopes");
  }
  return out_.str();
}

void JsonWriter::beforeValue() {
  if (scopes_.empty()) {
    return;
  }
  Scope &scope = scopes_.back();
  if (scope.kind == ScopeKind::Object) {
    if (!scope.waitingForValue) {
      throw ExecutionError("json writer value without key");
    }
    return;
  }
  commaIfNeeded();
}

void JsonWriter::afterValue() {
  if (scopes_.empty()) {
    return;
  }
  Scope &scope = scopes_.back();
  if (scope.kind == ScopeKind::Object) {
    scope.waitingForValue = false;
    scope.first = false;
  } else {
    scope.first = false;
  }
}

void JsonWriter::commaIfNeeded() {
  if (scopes_.empty()) {
    return;
  }
  Scope &scope = scopes_.back();
  if (!scope.first) {
    out_.append(',');
  }
}

std::string JsonWriter::escape(const std::string &value) {
  std::ostringstream out;
  out << '"';
  for (char ch : value) {
    switch (ch) {
    case '\\':
      out << "\\\\";
      break;
    case '"':
      out << "\\\"";
      break;
    case '\b':
      out << "\\b";
      break;
    case '\f':
      out << "\\f";
      break;
    case '\n':
      out << "\\n";
      break;
    case '\r':
      out << "\\r";
      break;
    case '\t':
      out << "\\t";
      break;
    default:
      if (static_cast<unsigned char>(ch) < 0x20U) {
        out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(ch));
      } else {
        out << ch;
      }
      break;
    }
  }
  out << '"';
  return out.str();
}

} // namespace aurora
