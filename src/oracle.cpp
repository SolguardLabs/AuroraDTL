#include "oracle.hpp"

#include <cmath>
#include <iomanip>

namespace aurora {

void OracleBook::seed(const PricePoint &point, const AssetRegistry &assets) {
  require(assets.has(point.asset), "oracle seed references unknown asset: " + point.asset);
  require(point.price > 0.0L, "oracle seed price must be positive: " + point.asset);
  require(point.confidenceBps <= 10000U, "oracle seed confidence out of range: " + point.asset);
  const auto inserted = prices_.emplace(point.asset, point);
  if (!inserted.second) {
    throw ValidationError("duplicate oracle price: " + point.asset);
  }
}

OracleDecision OracleBook::apply(const PriceUpdate &update, const AssetRegistry &assets, const Clock &clock) {
  require(assets.has(update.asset), "price update references unknown asset: " + update.asset);
  require(update.price > 0.0L, "price update must be positive: " + update.asset);
  require(update.confidenceBps <= 10000U, "price update confidence out of range: " + update.asset);
  const Asset &asset = assets.get(update.asset);
  OracleDecision decision;
  decision.asset = update.asset;
  decision.nextPrice = update.price;
  decision.nextSequence = update.sequence;
  auto it = prices_.find(update.asset);
  if (it == prices_.end()) {
    PricePoint point;
    point.asset = update.asset;
    point.price = update.price;
    point.confidenceBps = update.confidenceBps;
    point.updatedAt = update.updatedAt == 0U ? clock.timestamp : update.updatedAt;
    point.sequence = update.sequence;
    point.source = update.source;
    prices_.emplace(update.asset, point);
    decision.accepted = true;
    decision.reason = "seeded";
    return decision;
  }
  PricePoint &current = it->second;
  decision.previousPrice = current.price;
  decision.previousSequence = current.sequence;
  if (update.sequence <= current.sequence) {
    decision.accepted = false;
    decision.reason = "stale-sequence";
    return decision;
  }
  decision.deviationBps = priceDeviationBps(current.price, update.price);
  if (asset.maxDeviationBps != 0U && decision.deviationBps > asset.maxDeviationBps) {
    decision.accepted = false;
    decision.reason = "deviation-limit";
    return decision;
  }
  current.price = update.price;
  current.confidenceBps = update.confidenceBps;
  current.updatedAt = update.updatedAt == 0U ? clock.timestamp : update.updatedAt;
  current.sequence = update.sequence;
  current.source = update.source;
  decision.accepted = true;
  decision.reason = "accepted";
  return decision;
}

const PricePoint &OracleBook::priceOf(const std::string &asset) const {
  return lookup(prices_, asset, "price");
}

long double OracleBook::numericPrice(const std::string &asset) const {
  return priceOf(asset).price;
}

bool OracleBook::hasPrice(const std::string &asset) const {
  return prices_.find(asset) != prices_.end();
}

std::vector<PricePoint> OracleBook::prices() const {
  std::vector<PricePoint> result;
  result.reserve(prices_.size());
  for (const auto &entry : prices_) {
    result.push_back(entry.second);
  }
  return result;
}

void OracleBook::validateFresh(const std::string &assetSymbol, const AssetRegistry &assets, const Clock &clock) const {
  const Asset &asset = assets.get(assetSymbol);
  const PricePoint &point = priceOf(assetSymbol);
  if (asset.staleAfterSeconds == 0U) {
    return;
  }
  if (clock.timestamp > point.updatedAt && clock.timestamp - point.updatedAt > asset.staleAfterSeconds) {
    throw ValidationError("oracle price is stale for asset: " + assetSymbol);
  }
}

long double OracleBook::parsePrice(const std::string &value, const std::string &field) {
  const std::string normalized = trim(value);
  if (normalized.empty()) {
    throw ParseError("empty price for " + field);
  }
  char *end = nullptr;
  const long double parsed = std::strtold(normalized.c_str(), &end);
  if (end == normalized.c_str() || *end != '\0' || !std::isfinite(static_cast<double>(parsed)) || parsed <= 0.0L) {
    throw ParseError("invalid price for " + field + ": " + value);
  }
  return parsed;
}

std::string OracleBook::formatPrice(long double price) {
  std::ostringstream out;
  out << std::fixed << std::setprecision(8) << static_cast<double>(price);
  std::string text = out.str();
  while (!text.empty() && text.back() == '0') {
    text.pop_back();
  }
  if (!text.empty() && text.back() == '.') {
    text.pop_back();
  }
  if (text.empty()) {
    return "0";
  }
  return text;
}

std::uint32_t priceDeviationBps(long double previousPrice, long double nextPrice) {
  if (previousPrice <= 0.0L || nextPrice <= 0.0L) {
    return 10000U;
  }
  const long double high = std::max(previousPrice, nextPrice);
  const long double low = std::min(previousPrice, nextPrice);
  const long double bps = (high - low) * 10000.0L / high;
  if (bps < 0.0L) {
    return 0U;
  }
  if (bps > 10000.0L) {
    return 10000U;
  }
  return static_cast<std::uint32_t>(std::floor(bps + 0.5L));
}

} // namespace aurora
