#pragma once

#include "common.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace aurora {

struct IdParts {
  std::string namespaceId;
  std::string primary;
  std::string secondary;
  std::uint64_t sequence{0};
};

class StableId {
public:
  StableId();
  explicit StableId(std::string value);

  const std::string &value() const;
  bool empty() const;
  std::string str() const;

  bool operator==(const StableId &other) const;
  bool operator!=(const StableId &other) const;
  bool operator<(const StableId &other) const;

private:
  std::string value_;
};

class IdGenerator {
public:
  explicit IdGenerator(std::string namespaceId);

  StableId next(const std::string &primary);
  StableId scoped(const std::string &primary, const std::string &secondary, std::uint64_t sequence) const;
  StableId fromParts(const IdParts &parts) const;
  std::uint64_t counter() const;

private:
  std::string namespaceId_;
  std::uint64_t counter_{0};
};

std::uint64_t fnv1a64(const std::string &value);
std::string hex64(std::uint64_t value);
std::string compactId(const std::vector<std::string> &parts);
std::string sanitizeIdPart(const std::string &part);

} // namespace aurora
