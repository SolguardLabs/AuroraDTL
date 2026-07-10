#pragma once

#include "asset.hpp"

#include <map>
#include <string>

namespace aurora {

struct FeeBreakdown {
  std::uint32_t baseBps{0};
  std::uint32_t routeBps{0};
  std::uint32_t windowBps{0};
  std::uint32_t assetBps{0};
  std::uint32_t totalBps{0};
  Amount fee{Amount::zero()};
  Amount net{Amount::zero()};
};

struct AssetFeeOverride {
  std::string asset;
  std::uint32_t bps{0};
};

class FeeSchedule {
public:
  void setDefaultBps(std::uint32_t bps);
  void setRouteBps(std::uint32_t bps);
  void setWindowBps(std::uint32_t bps);
  void setFeeAccount(std::string account);
  void addAssetOverride(const std::string &asset, std::uint32_t bps);

  std::uint32_t defaultBps() const;
  std::uint32_t routeBps() const;
  std::uint32_t windowBps() const;
  const std::string &feeAccount() const;
  std::uint32_t assetOverride(const std::string &asset) const;
  FeeBreakdown quote(Amount gross, const std::string &asset, std::uint32_t routeExtraBps, std::uint32_t windowExtraBps) const;

private:
  std::uint32_t defaultBps_{0};
  std::uint32_t routeBps_{0};
  std::uint32_t windowBps_{0};
  std::string feeAccount_{"treasury"};
  std::map<std::string, std::uint32_t> assetOverrides_;
};

} // namespace aurora
