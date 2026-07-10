#include "fees.hpp"

namespace aurora {

void FeeSchedule::setDefaultBps(std::uint32_t bps) {
  require(bps <= 10000U, "default fee bps out of range");
  defaultBps_ = bps;
}

void FeeSchedule::setRouteBps(std::uint32_t bps) {
  require(bps <= 10000U, "route fee bps out of range");
  routeBps_ = bps;
}

void FeeSchedule::setWindowBps(std::uint32_t bps) {
  require(bps <= 10000U, "window fee bps out of range");
  windowBps_ = bps;
}

void FeeSchedule::setFeeAccount(std::string account) {
  require(isIdentifier(account), "fee account is not a valid identifier: " + account);
  feeAccount_ = std::move(account);
}

void FeeSchedule::addAssetOverride(const std::string &asset, std::uint32_t bps) {
  require(isIdentifier(asset), "asset fee override symbol is invalid: " + asset);
  require(bps <= 10000U, "asset fee override bps out of range: " + asset);
  assetOverrides_[asset] = bps;
}

std::uint32_t FeeSchedule::defaultBps() const {
  return defaultBps_;
}

std::uint32_t FeeSchedule::routeBps() const {
  return routeBps_;
}

std::uint32_t FeeSchedule::windowBps() const {
  return windowBps_;
}

const std::string &FeeSchedule::feeAccount() const {
  return feeAccount_;
}

std::uint32_t FeeSchedule::assetOverride(const std::string &asset) const {
  auto it = assetOverrides_.find(asset);
  if (it == assetOverrides_.end()) {
    return 0U;
  }
  return it->second;
}

FeeBreakdown FeeSchedule::quote(Amount gross, const std::string &asset, std::uint32_t routeExtraBps, std::uint32_t windowExtraBps) const {
  FeeBreakdown breakdown;
  breakdown.baseBps = defaultBps_;
  breakdown.routeBps = routeBps_ + routeExtraBps;
  breakdown.windowBps = windowBps_ + windowExtraBps;
  breakdown.assetBps = assetOverride(asset);
  const std::uint64_t total =
      static_cast<std::uint64_t>(breakdown.baseBps) +
      static_cast<std::uint64_t>(breakdown.routeBps) +
      static_cast<std::uint64_t>(breakdown.windowBps) +
      static_cast<std::uint64_t>(breakdown.assetBps);
  breakdown.totalBps = total > 10000U ? 10000U : static_cast<std::uint32_t>(total);
  breakdown.fee = gross.applyBps(breakdown.totalBps);
  breakdown.net = gross.checkedSub(breakdown.fee, "fee net amount");
  return breakdown;
}

} // namespace aurora
