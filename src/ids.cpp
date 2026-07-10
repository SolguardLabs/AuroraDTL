#include "ids.hpp"

#include <iomanip>

namespace aurora {

StableId::StableId() = default;

StableId::StableId(std::string value) : value_(std::move(value)) {
  require(value_.empty() || isIdentifier(value_), "stable id is not a valid identifier: " + value_);
}

const std::string &StableId::value() const {
  return value_;
}

bool StableId::empty() const {
  return value_.empty();
}

std::string StableId::str() const {
  return value_;
}

bool StableId::operator==(const StableId &other) const {
  return value_ == other.value_;
}

bool StableId::operator!=(const StableId &other) const {
  return value_ != other.value_;
}

bool StableId::operator<(const StableId &other) const {
  return value_ < other.value_;
}

IdGenerator::IdGenerator(std::string namespaceId) : namespaceId_(std::move(namespaceId)) {
  require(isIdentifier(namespaceId_), "id namespace is not a valid identifier: " + namespaceId_);
}

StableId IdGenerator::next(const std::string &primary) {
  ++counter_;
  return scoped(primary, "seq", counter_);
}

StableId IdGenerator::scoped(const std::string &primary, const std::string &secondary, std::uint64_t sequence) const {
  IdParts parts;
  parts.namespaceId = namespaceId_;
  parts.primary = primary;
  parts.secondary = secondary;
  parts.sequence = sequence;
  return fromParts(parts);
}

StableId IdGenerator::fromParts(const IdParts &parts) const {
  const std::string primary = sanitizeIdPart(parts.primary);
  const std::string secondary = sanitizeIdPart(parts.secondary);
  const std::string base = compactId({parts.namespaceId.empty() ? namespaceId_ : parts.namespaceId, primary, secondary, std::to_string(parts.sequence)});
  const std::string hash = hex64(fnv1a64(base)).substr(0U, 12U);
  return StableId(base + "-" + hash);
}

std::uint64_t IdGenerator::counter() const {
  return counter_;
}

std::uint64_t fnv1a64(const std::string &value) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (unsigned char ch : value) {
    hash ^= static_cast<std::uint64_t>(ch);
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setw(16) << std::setfill('0') << value;
  return out.str();
}

std::string compactId(const std::vector<std::string> &parts) {
  std::vector<std::string> clean;
  clean.reserve(parts.size());
  for (const std::string &part : parts) {
    const std::string sanitized = sanitizeIdPart(part);
    if (!sanitized.empty()) {
      clean.push_back(sanitized);
    }
  }
  if (clean.empty()) {
    return "id";
  }
  return join(clean, "-");
}

std::string sanitizeIdPart(const std::string &part) {
  std::string result;
  result.reserve(part.size());
  bool previousDash = false;
  for (char ch : lower(part)) {
    const bool keep = std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_' || ch == '.';
    if (keep) {
      result.push_back(ch);
      previousDash = false;
    } else if (!previousDash) {
      result.push_back('-');
      previousDash = true;
    }
  }
  while (!result.empty() && result.front() == '-') {
    result.erase(result.begin());
  }
  while (!result.empty() && result.back() == '-') {
    result.pop_back();
  }
  if (result.size() > 48U) {
    result.resize(48U);
    while (!result.empty() && result.back() == '-') {
      result.pop_back();
    }
  }
  return result;
}

} // namespace aurora
