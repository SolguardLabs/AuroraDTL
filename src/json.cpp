#include "json.hpp"

#include <fstream>

namespace aurora {

JsonValue::JsonValue() = default;

JsonValue::JsonValue(std::nullptr_t) : type_(Type::Null) {}

JsonValue::JsonValue(bool value) : type_(Type::Bool), boolValue_(value) {}

JsonValue::JsonValue(std::string value, bool numberToken) : type_(numberToken ? Type::Number : Type::String), text_(std::move(value)) {}

JsonValue JsonValue::array(std::vector<JsonValue> values) {
  JsonValue result;
  result.type_ = Type::Array;
  result.array_ = std::move(values);
  return result;
}

JsonValue JsonValue::object(std::map<std::string, JsonValue> values) {
  JsonValue result;
  result.type_ = Type::Object;
  result.object_ = std::move(values);
  return result;
}

JsonValue::Type JsonValue::type() const {
  return type_;
}

bool JsonValue::isNull() const {
  return type_ == Type::Null;
}

bool JsonValue::isBool() const {
  return type_ == Type::Bool;
}

bool JsonValue::isNumber() const {
  return type_ == Type::Number;
}

bool JsonValue::isString() const {
  return type_ == Type::String;
}

bool JsonValue::isArray() const {
  return type_ == Type::Array;
}

bool JsonValue::isObject() const {
  return type_ == Type::Object;
}

bool JsonValue::asBool(const std::string &field) const {
  if (!isBool()) {
    throw ParseError(field + " must be a boolean");
  }
  return boolValue_;
}

const std::string &JsonValue::asString(const std::string &field) const {
  if (!isString()) {
    throw ParseError(field + " must be a string");
  }
  return text_;
}

const std::string &JsonValue::asNumberToken(const std::string &field) const {
  if (!isNumber()) {
    throw ParseError(field + " must be a number");
  }
  return text_;
}

std::uint64_t JsonValue::asUint64(const std::string &field) const {
  if (isString() || isNumber()) {
    return parseUint64(text_, field);
  }
  throw ParseError(field + " must be an unsigned integer");
}

std::int64_t JsonValue::asInt64(const std::string &field) const {
  if (isString() || isNumber()) {
    return parseInt64(text_, field);
  }
  throw ParseError(field + " must be an integer");
}

const std::vector<JsonValue> &JsonValue::asArray(const std::string &field) const {
  if (!isArray()) {
    throw ParseError(field + " must be an array");
  }
  return array_;
}

const std::map<std::string, JsonValue> &JsonValue::asObject(const std::string &field) const {
  if (!isObject()) {
    throw ParseError(field + " must be an object");
  }
  return object_;
}

bool JsonValue::has(const std::string &key) const {
  if (!isObject()) {
    return false;
  }
  return object_.find(key) != object_.end();
}

const JsonValue &JsonValue::at(const std::string &key) const {
  if (!isObject()) {
    throw ParseError("json value is not an object");
  }
  auto it = object_.find(key);
  if (it == object_.end()) {
    throw ParseError("missing json field: " + key);
  }
  return it->second;
}

const JsonValue *JsonValue::maybe(const std::string &key) const {
  if (!isObject()) {
    return nullptr;
  }
  auto it = object_.find(key);
  if (it == object_.end()) {
    return nullptr;
  }
  return &it->second;
}

class JsonParser {
public:
  explicit JsonParser(const std::string &text) : text_(text) {}

  JsonValue parse() {
    skipWhitespace();
    JsonValue value = parseValue();
    skipWhitespace();
    if (!eof()) {
      fail("unexpected trailing data");
    }
    return value;
  }

private:
  const std::string &text_;
  std::size_t offset_{0};

  bool eof() const {
    return offset_ >= text_.size();
  }

  char peek() const {
    if (eof()) {
      return '\0';
    }
    return text_[offset_];
  }

  char get() {
    if (eof()) {
      fail("unexpected end of input");
    }
    const char ch = text_[offset_];
    ++offset_;
    return ch;
  }

  void skipWhitespace() {
    while (!eof()) {
      const char ch = peek();
      if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') {
        ++offset_;
      } else {
        break;
      }
    }
  }

  void expect(char expected) {
    const char actual = get();
    if (actual != expected) {
      std::string message = "expected '";
      message.push_back(expected);
      message += "' but found '";
      message.push_back(actual);
      message += "'";
      fail(message);
    }
  }

  JsonValue parseValue() {
    skipWhitespace();
    const char ch = peek();
    if (ch == '{') {
      return parseObject();
    }
    if (ch == '[') {
      return parseArray();
    }
    if (ch == '"') {
      return JsonValue(parseString());
    }
    if (ch == 't') {
      consumeLiteral("true");
      return JsonValue(true);
    }
    if (ch == 'f') {
      consumeLiteral("false");
      return JsonValue(false);
    }
    if (ch == 'n') {
      consumeLiteral("null");
      return JsonValue(nullptr);
    }
    if (ch == '-' || std::isdigit(static_cast<unsigned char>(ch)) != 0) {
      return JsonValue(parseNumber(), true);
    }
    fail("unexpected character while parsing value");
    return JsonValue();
  }

  JsonValue parseObject() {
    expect('{');
    skipWhitespace();
    std::map<std::string, JsonValue> values;
    if (peek() == '}') {
      get();
      return JsonValue::object(std::move(values));
    }
    while (true) {
      skipWhitespace();
      if (peek() != '"') {
        fail("object key must be a string");
      }
      std::string key = parseString();
      skipWhitespace();
      expect(':');
      JsonValue value = parseValue();
      auto inserted = values.emplace(std::move(key), std::move(value));
      if (!inserted.second) {
        fail("duplicate object key");
      }
      skipWhitespace();
      const char ch = get();
      if (ch == '}') {
        break;
      }
      if (ch != ',') {
        fail("expected ',' or '}' in object");
      }
    }
    return JsonValue::object(std::move(values));
  }

  JsonValue parseArray() {
    expect('[');
    skipWhitespace();
    std::vector<JsonValue> values;
    if (peek() == ']') {
      get();
      return JsonValue::array(std::move(values));
    }
    while (true) {
      values.push_back(parseValue());
      skipWhitespace();
      const char ch = get();
      if (ch == ']') {
        break;
      }
      if (ch != ',') {
        fail("expected ',' or ']' in array");
      }
    }
    return JsonValue::array(std::move(values));
  }

  std::string parseString() {
    expect('"');
    std::string result;
    while (true) {
      const char ch = get();
      if (ch == '"') {
        break;
      }
      if (ch == '\\') {
        result.push_back(parseEscape());
      } else {
        if (static_cast<unsigned char>(ch) < 0x20U) {
          fail("control character in string");
        }
        result.push_back(ch);
      }
    }
    return result;
  }

  char parseEscape() {
    const char escaped = get();
    switch (escaped) {
    case '"':
      return '"';
    case '\\':
      return '\\';
    case '/':
      return '/';
    case 'b':
      return '\b';
    case 'f':
      return '\f';
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    case 'u':
      return parseUnicodeEscape();
    default:
      fail("invalid escape sequence");
      return '\0';
    }
  }

  char parseUnicodeEscape() {
    unsigned value = 0U;
    for (int i = 0; i < 4; ++i) {
      const char ch = get();
      value <<= 4U;
      if (ch >= '0' && ch <= '9') {
        value += static_cast<unsigned>(ch - '0');
      } else if (ch >= 'a' && ch <= 'f') {
        value += static_cast<unsigned>(10 + ch - 'a');
      } else if (ch >= 'A' && ch <= 'F') {
        value += static_cast<unsigned>(10 + ch - 'A');
      } else {
        fail("invalid unicode escape");
      }
    }
    if (value <= 0x7FU) {
      return static_cast<char>(value);
    }
    return '?';
  }

  std::string parseNumber() {
    const std::size_t start = offset_;
    if (peek() == '-') {
      ++offset_;
    }
    consumeDigits();
    if (peek() == '.') {
      ++offset_;
      consumeDigits();
    }
    if (peek() == 'e' || peek() == 'E') {
      ++offset_;
      if (peek() == '+' || peek() == '-') {
        ++offset_;
      }
      consumeDigits();
    }
    return text_.substr(start, offset_ - start);
  }

  void consumeDigits() {
    if (std::isdigit(static_cast<unsigned char>(peek())) == 0) {
      fail("expected digit");
    }
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
      ++offset_;
    }
  }

  void consumeLiteral(const std::string &literal) {
    for (char expected : literal) {
      if (get() != expected) {
        fail("invalid literal");
      }
    }
  }

  [[noreturn]] void fail(const std::string &message) const {
    std::ostringstream out;
    out << "json parse error at byte " << offset_ << ": " << message;
    throw ParseError(out.str());
  }
};

JsonValue parseJsonText(const std::string &text) {
  JsonParser parser(text);
  return parser.parse();
}

JsonValue parseJsonFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw ParseError("unable to open fixture: " + path);
  }
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return parseJsonText(buffer.str());
}

std::string jsonRequiredString(const JsonValue &object, const std::string &field) {
  return object.at(field).asString(field);
}

std::string jsonOptionalString(const JsonValue &object, const std::string &field, const std::string &fallback) {
  const JsonValue *value = object.maybe(field);
  if (value == nullptr || value->isNull()) {
    return fallback;
  }
  return value->asString(field);
}

std::uint64_t jsonRequiredUint64(const JsonValue &object, const std::string &field) {
  return object.at(field).asUint64(field);
}

std::uint64_t jsonOptionalUint64(const JsonValue &object, const std::string &field, std::uint64_t fallback) {
  const JsonValue *value = object.maybe(field);
  if (value == nullptr || value->isNull()) {
    return fallback;
  }
  return value->asUint64(field);
}

bool jsonOptionalBool(const JsonValue &object, const std::string &field, bool fallback) {
  const JsonValue *value = object.maybe(field);
  if (value == nullptr || value->isNull()) {
    return fallback;
  }
  return value->asBool(field);
}

std::vector<std::string> jsonStringArray(const JsonValue &object, const std::string &field) {
  std::vector<std::string> result;
  const auto &values = object.at(field).asArray(field);
  result.reserve(values.size());
  for (const JsonValue &value : values) {
    result.push_back(value.asString(field));
  }
  return result;
}

} // namespace aurora
