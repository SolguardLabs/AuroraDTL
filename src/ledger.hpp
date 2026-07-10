#pragma once

#include "asset.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct BalanceRow {
  std::string account;
  std::string asset;
  Amount amount{Amount::zero()};
};

struct LedgerEvent {
  std::string type;
  std::string account;
  std::string asset;
  Amount amount{Amount::zero()};
  std::string ref;
};

class Ledger {
public:
  void ensureAccount(const std::string &account);
  void credit(const std::string &account, const std::string &asset, Amount amount, const std::string &ref);
  void debit(const std::string &account, const std::string &asset, Amount amount, const std::string &ref);
  Amount balanceOf(const std::string &account, const std::string &asset) const;
  bool hasAccount(const std::string &account) const;
  std::vector<BalanceRow> balances() const;
  const std::vector<LedgerEvent> &events() const;
  void validateAssets(const AssetRegistry &assets) const;

private:
  using AssetBalances = std::map<std::string, Amount>;

  std::map<std::string, AssetBalances> balances_;
  std::vector<LedgerEvent> events_;
};

} // namespace aurora
