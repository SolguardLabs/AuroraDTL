#include "ledger.hpp"

namespace aurora {

void Ledger::ensureAccount(const std::string &account) {
  require(isIdentifier(account), "account id is not a valid identifier: " + account);
  balances_.try_emplace(account);
}

void Ledger::credit(const std::string &account, const std::string &asset, Amount amount, const std::string &ref) {
  ensureAccount(account);
  AssetBalances &row = balances_[account];
  const Amount current = row[asset];
  row[asset] = current.checkedAdd(amount, "ledger credit");
  events_.push_back(LedgerEvent{"credit", account, asset, amount, ref});
}

void Ledger::debit(const std::string &account, const std::string &asset, Amount amount, const std::string &ref) {
  ensureAccount(account);
  AssetBalances &row = balances_[account];
  const Amount current = row[asset];
  if (current < amount) {
    throw ExecutionError("insufficient balance for " + account + " in " + asset);
  }
  row[asset] = current.checkedSub(amount, "ledger debit");
  events_.push_back(LedgerEvent{"debit", account, asset, amount, ref});
}

Amount Ledger::balanceOf(const std::string &account, const std::string &asset) const {
  auto accountIt = balances_.find(account);
  if (accountIt == balances_.end()) {
    return Amount::zero();
  }
  auto assetIt = accountIt->second.find(asset);
  if (assetIt == accountIt->second.end()) {
    return Amount::zero();
  }
  return assetIt->second;
}

bool Ledger::hasAccount(const std::string &account) const {
  return balances_.find(account) != balances_.end();
}

std::vector<BalanceRow> Ledger::balances() const {
  std::vector<BalanceRow> result;
  for (const auto &accountEntry : balances_) {
    for (const auto &assetEntry : accountEntry.second) {
      result.push_back(BalanceRow{accountEntry.first, assetEntry.first, assetEntry.second});
    }
  }
  return result;
}

const std::vector<LedgerEvent> &Ledger::events() const {
  return events_;
}

void Ledger::validateAssets(const AssetRegistry &assets) const {
  for (const auto &accountEntry : balances_) {
    require(isIdentifier(accountEntry.first), "ledger account id is invalid: " + accountEntry.first);
    for (const auto &assetEntry : accountEntry.second) {
      require(assets.has(assetEntry.first), "ledger references unknown asset: " + assetEntry.first);
    }
  }
}

} // namespace aurora
