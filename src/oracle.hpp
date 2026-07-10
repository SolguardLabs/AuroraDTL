#pragma once

#include "asset.hpp"
#include "common.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct PricePoint {
  std::string asset;
  long double price{0.0L};
  std::uint32_t confidenceBps{0};
  std::uint64_t updatedAt{0};
  std::uint64_t sequence{0};
  std::string source;
};

struct PriceUpdate {
  std::string asset;
  long double price{0.0L};
  std::uint32_t confidenceBps{0};
  std::uint64_t updatedAt{0};
  std::uint64_t sequence{0};
  std::string source;
};

struct OracleDecision {
  bool accepted{false};
  std::string asset;
  std::string reason;
  std::uint32_t deviationBps{0};
  std::uint64_t previousSequence{0};
  std::uint64_t nextSequence{0};
  long double previousPrice{0.0L};
  long double nextPrice{0.0L};
};

class OracleBook {
public:
  void seed(const PricePoint &point, const AssetRegistry &assets);
  OracleDecision apply(const PriceUpdate &update, const AssetRegistry &assets, const Clock &clock);
  const PricePoint &priceOf(const std::string &asset) const;
  long double numericPrice(const std::string &asset) const;
  bool hasPrice(const std::string &asset) const;
  std::vector<PricePoint> prices() const;
  void validateFresh(const std::string &asset, const AssetRegistry &assets, const Clock &clock) const;
  static long double parsePrice(const std::string &value, const std::string &field);
  static std::string formatPrice(long double price);

private:
  std::map<std::string, PricePoint> prices_;
};

std::uint32_t priceDeviationBps(long double previousPrice, long double nextPrice);

} // namespace aurora
