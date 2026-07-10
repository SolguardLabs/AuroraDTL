#pragma once

#include "amount.hpp"
#include "common.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

enum class AssetStatus { Active, Paused, WithdrawOnly };

std::string assetStatusText(AssetStatus status);
AssetStatus parseAssetStatus(const std::string &value);

struct Asset {
  std::string symbol;
  std::string displayName;
  std::uint8_t decimals{0};
  AssetStatus status{AssetStatus::Active};
  std::uint32_t maxDeviationBps{0};
  std::uint32_t defaultToleranceBps{0};
  std::uint64_t staleAfterSeconds{0};
  Amount minTrade{Amount::zero()};
  Amount maxTrade{Amount::zero()};
  bool settlementEnabled{true};
};

class AssetRegistry {
public:
  void add(Asset asset);
  bool has(const std::string &symbol) const;
  const Asset &get(const std::string &symbol) const;
  Asset &getMutable(const std::string &symbol);
  const std::map<std::string, Asset> &all() const;
  std::vector<std::string> symbols() const;
  void validateTradable(const std::string &symbol, Amount amount) const;
  void validateSettlement(const std::string &symbol) const;
  std::uint32_t toleranceFor(const std::string &symbol, std::uint32_t routeTolerance) const;

private:
  std::map<std::string, Asset> assets_;
};

} // namespace aurora
