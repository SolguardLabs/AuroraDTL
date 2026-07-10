#include "reconcile.hpp"

namespace aurora {

bool ReconcileReport::clean() const {
  return findings.empty();
}

ReconcileReport Reconciler::reconcile(const std::vector<SettlementResult> &settlements, const std::vector<LedgerEvent> &ledgerEvents) const {
  ReconcileReport report;
  report.assetTotals = aggregateAssets(ledgerEvents);
  report.accountDeltas = aggregateAccounts(ledgerEvents);
  report.settlementAggregates = aggregateSettlements(settlements);
  report.findings = compare(report.settlementAggregates, report.accountDeltas);
  return report;
}

std::vector<AssetTotal> Reconciler::aggregateAssets(const std::vector<LedgerEvent> &events) const {
  std::map<std::string, AssetTotal> totals;
  for (const LedgerEvent &event : events) {
    AssetTotal &total = totals[event.asset];
    total.asset = event.asset;
    if (event.type == "debit") {
      total.debits = total.debits.checkedAdd(event.amount, "reconcile asset debits");
    } else if (event.type == "credit") {
      total.credits = total.credits.checkedAdd(event.amount, "reconcile asset credits");
    }
  }
  std::vector<AssetTotal> result;
  result.reserve(totals.size());
  for (auto &entry : totals) {
    AssetTotal &total = entry.second;
    if (total.credits >= total.debits) {
      total.net = total.credits.checkedSub(total.debits, "reconcile asset net");
    } else {
      total.net = total.debits.checkedSub(total.credits, "reconcile asset net");
    }
    result.push_back(total);
  }
  return result;
}

std::vector<AccountDelta> Reconciler::aggregateAccounts(const std::vector<LedgerEvent> &events) const {
  std::map<std::string, AccountDelta> deltas;
  for (const LedgerEvent &event : events) {
    const std::string key = accountAssetKey(event.account, event.asset);
    AccountDelta &delta = deltas[key];
    delta.account = event.account;
    delta.asset = event.asset;
    if (event.type == "debit") {
      delta.debits = delta.debits.checkedAdd(event.amount, "reconcile account debits");
    } else if (event.type == "credit") {
      delta.credits = delta.credits.checkedAdd(event.amount, "reconcile account credits");
    }
  }
  std::vector<AccountDelta> result;
  result.reserve(deltas.size());
  for (const auto &entry : deltas) {
    result.push_back(entry.second);
  }
  return result;
}

std::vector<SettlementAggregate> Reconciler::aggregateSettlements(const std::vector<SettlementResult> &settlements) const {
  std::map<std::string, SettlementAggregate> aggregates;
  for (const SettlementResult &settlement : settlements) {
    const std::string key = routeAssetKey(settlement.quote.routeId, settlement.quote.destinationAsset);
    SettlementAggregate &aggregate = aggregates[key];
    aggregate.routeId = settlement.quote.routeId;
    aggregate.destinationAsset = settlement.quote.destinationAsset;
    if (settlement.status == "settled") {
      ++aggregate.settled;
      aggregate.sourceDebit = aggregate.sourceDebit.checkedAdd(settlement.sourceDebit, "reconcile settlement source");
      aggregate.destinationCredit = aggregate.destinationCredit.checkedAdd(settlement.destinationCredit, "reconcile settlement destination");
      aggregate.fees = aggregate.fees.checkedAdd(settlement.destinationFee, "reconcile settlement fees");
    } else {
      ++aggregate.rejected;
    }
  }
  std::vector<SettlementAggregate> result;
  result.reserve(aggregates.size());
  for (const auto &entry : aggregates) {
    result.push_back(entry.second);
  }
  return result;
}

std::vector<ReconcileFinding> Reconciler::compare(
    const std::vector<SettlementAggregate> &settlements,
    const std::vector<AccountDelta> &accounts) const {
  std::vector<ReconcileFinding> findings;
  std::map<std::string, Amount> creditedByAsset;
  std::map<std::string, Amount> debitedByAsset;
  for (const AccountDelta &delta : accounts) {
    creditedByAsset[delta.asset] = creditedByAsset[delta.asset].checkedAdd(delta.credits, "reconcile credited by asset");
    debitedByAsset[delta.asset] = debitedByAsset[delta.asset].checkedAdd(delta.debits, "reconcile debited by asset");
  }
  for (const SettlementAggregate &aggregate : settlements) {
    if (aggregate.settled == 0U) {
      continue;
    }
    const Amount expectedCredit = aggregate.destinationCredit.checkedAdd(aggregate.fees, "reconcile expected credit");
    const Amount observedCredit = creditedByAsset[aggregate.destinationAsset];
    if (observedCredit < expectedCredit) {
      findings.push_back(ReconcileFinding{
          "settlement.credit-shortfall",
          aggregate.routeId,
          "ledger credits below settlement aggregate for destination asset"});
    }
  }
  for (const auto &entry : creditedByAsset) {
    const Amount credits = entry.second;
    const Amount debits = debitedByAsset[entry.first];
    if (credits.isZero() && debits.isZero()) {
      continue;
    }
    if (credits > Amount(0) && debits > Amount(0)) {
      continue;
    }
  }
  return findings;
}

std::string Reconciler::accountAssetKey(const std::string &account, const std::string &asset) {
  return account + "\n" + asset;
}

std::string Reconciler::routeAssetKey(const std::string &route, const std::string &asset) {
  return route + "\n" + asset;
}

} // namespace aurora
