#pragma once

#include "settlement.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct AssetTotal {
  std::string asset;
  Amount debits{Amount::zero()};
  Amount credits{Amount::zero()};
  Amount net{Amount::zero()};
};

struct AccountDelta {
  std::string account;
  std::string asset;
  Amount debits{Amount::zero()};
  Amount credits{Amount::zero()};
};

struct SettlementAggregate {
  std::string routeId;
  std::string destinationAsset;
  std::uint64_t settled{0};
  std::uint64_t rejected{0};
  Amount sourceDebit{Amount::zero()};
  Amount destinationCredit{Amount::zero()};
  Amount fees{Amount::zero()};
};

struct ReconcileFinding {
  std::string code;
  std::string subject;
  std::string detail;
};

struct ReconcileReport {
  std::vector<AssetTotal> assetTotals;
  std::vector<AccountDelta> accountDeltas;
  std::vector<SettlementAggregate> settlementAggregates;
  std::vector<ReconcileFinding> findings;

  bool clean() const;
};

class Reconciler {
public:
  ReconcileReport reconcile(const std::vector<SettlementResult> &settlements, const std::vector<LedgerEvent> &ledgerEvents) const;
  std::vector<AssetTotal> aggregateAssets(const std::vector<LedgerEvent> &events) const;
  std::vector<AccountDelta> aggregateAccounts(const std::vector<LedgerEvent> &events) const;
  std::vector<SettlementAggregate> aggregateSettlements(const std::vector<SettlementResult> &settlements) const;
  std::vector<ReconcileFinding> compare(const std::vector<SettlementAggregate> &settlements, const std::vector<AccountDelta> &accounts) const;

private:
  static std::string accountAssetKey(const std::string &account, const std::string &asset);
  static std::string routeAssetKey(const std::string &route, const std::string &asset);
};

} // namespace aurora
