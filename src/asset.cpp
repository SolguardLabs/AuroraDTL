#include "asset.hpp"

namespace aurora {

std::string assetStatusText(AssetStatus status) {
  switch (status) {
  case AssetStatus::Active:
    return "active";
  case AssetStatus::Paused:
    return "paused";
  case AssetStatus::WithdrawOnly:
    return "withdrawOnly";
  }
  return "unknown";
}

AssetStatus parseAssetStatus(const std::string &value) {
  const std::string normalized = lower(value);
  if (normalized == "active") {
    return AssetStatus::Active;
  }
  if (normalized == "paused") {
    return AssetStatus::Paused;
  }
  if (normalized == "withdrawonly" || normalized == "withdraw-only" || normalized == "withdraw_only") {
    return AssetStatus::WithdrawOnly;
  }
  throw ValidationError("unknown asset status: " + value);
}

void AssetRegistry::add(Asset asset) {
  require(isIdentifier(asset.symbol), "asset symbol is not a valid identifier: " + asset.symbol);
  require(asset.decimals <= 18U, "asset decimals above 18 are not supported: " + asset.symbol);
  require(asset.maxDeviationBps <= 10000U, "asset max deviation bps out of range: " + asset.symbol);
  require(asset.defaultToleranceBps <= 10000U, "asset tolerance bps out of range: " + asset.symbol);
  if (!asset.maxTrade.isZero()) {
    require(asset.maxTrade >= asset.minTrade, "asset max trade below min trade: " + asset.symbol);
  }
  const auto inserted = assets_.emplace(asset.symbol, std::move(asset));
  if (!inserted.second) {
    throw ValidationError("duplicate asset: " + inserted.first->first);
  }
}

bool AssetRegistry::has(const std::string &symbol) const {
  return assets_.find(symbol) != assets_.end();
}

const Asset &AssetRegistry::get(const std::string &symbol) const {
  return lookup(assets_, symbol, "asset");
}

Asset &AssetRegistry::getMutable(const std::string &symbol) {
  return lookupMutable(assets_, symbol, "asset");
}

const std::map<std::string, Asset> &AssetRegistry::all() const {
  return assets_;
}

std::vector<std::string> AssetRegistry::symbols() const {
  std::vector<std::string> result;
  result.reserve(assets_.size());
  for (const auto &entry : assets_) {
    result.push_back(entry.first);
  }
  return result;
}

void AssetRegistry::validateTradable(const std::string &symbol, Amount amount) const {
  const Asset &asset = get(symbol);
  if (asset.status != AssetStatus::Active) {
    throw ValidationError("asset is not active: " + symbol);
  }
  if (!asset.minTrade.isZero() && amount < asset.minTrade) {
    throw ValidationError("trade below min size for asset: " + symbol);
  }
  if (!asset.maxTrade.isZero() && amount > asset.maxTrade) {
    throw ValidationError("trade above max size for asset: " + symbol);
  }
}

void AssetRegistry::validateSettlement(const std::string &symbol) const {
  const Asset &asset = get(symbol);
  if (!asset.settlementEnabled) {
    throw ValidationError("settlement disabled for asset: " + symbol);
  }
  if (asset.status == AssetStatus::Paused) {
    throw ValidationError("settlement paused for asset: " + symbol);
  }
}

std::uint32_t AssetRegistry::toleranceFor(const std::string &symbol, std::uint32_t routeTolerance) const {
  const Asset &asset = get(symbol);
  if (routeTolerance == 0U) {
    return asset.defaultToleranceBps;
  }
  if (asset.defaultToleranceBps == 0U) {
    return routeTolerance;
  }
  return std::min(routeTolerance, asset.defaultToleranceBps);
}

} // namespace aurora
