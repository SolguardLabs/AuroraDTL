#include "audit.hpp"

#include <sstream>

namespace aurora {

void AuditSummary::add(std::string section, std::string key, std::string value) {
  items.push_back(AuditItem{std::move(section), std::move(key), std::move(value)});
}

std::string AuditSummary::valueOf(const std::string &section, const std::string &key) const {
  for (const AuditItem &item : items) {
    if (item.section == section && item.key == key) {
      return item.value;
    }
  }
  return "";
}

CatalogAuditor::CatalogAuditor(
    const AssetRegistry &assets,
    const OracleBook &oracles,
    const RouteBook &routes,
    const FeeSchedule &fees)
    : assets_(assets), oracles_(oracles), routes_(routes), fees_(fees) {}

AuditSummary CatalogAuditor::summarize() const {
  AuditSummary summary;
  summarizeAssets(summary);
  summarizeRoutes(summary);
  summarizePrices(summary);
  summarizeFees(summary);
  return summary;
}

void CatalogAuditor::summarizeAssets(AuditSummary &summary) const {
  summary.add("assets", "count", std::to_string(assets_.all().size()));
  std::uint64_t active = 0U;
  std::uint64_t paused = 0U;
  std::uint64_t withdrawOnly = 0U;
  std::uint32_t highestTolerance = 0U;
  for (const auto &entry : assets_.all()) {
    const Asset &asset = entry.second;
    if (asset.status == AssetStatus::Active) {
      ++active;
    } else if (asset.status == AssetStatus::Paused) {
      ++paused;
    } else {
      ++withdrawOnly;
    }
    highestTolerance = std::max(highestTolerance, asset.defaultToleranceBps);
  }
  summary.add("assets", "active", std::to_string(active));
  summary.add("assets", "paused", std::to_string(paused));
  summary.add("assets", "withdrawOnly", std::to_string(withdrawOnly));
  summary.add("assets", "highestToleranceBps", std::to_string(highestTolerance));
}

void CatalogAuditor::summarizeRoutes(AuditSummary &summary) const {
  summary.add("routes", "count", std::to_string(routes_.all().size()));
  std::uint64_t enabled = 0U;
  std::uint64_t disabled = 0U;
  std::uint64_t maxHops = 0U;
  for (const auto &entry : routes_.all()) {
    const Route &route = entry.second;
    if (route.enabled) {
      ++enabled;
    } else {
      ++disabled;
    }
    const std::uint64_t hops = route.hops.size() > 0U ? static_cast<std::uint64_t>(route.hops.size() - 1U) : 0U;
    maxHops = std::max(maxHops, hops);
  }
  summary.add("routes", "enabled", std::to_string(enabled));
  summary.add("routes", "disabled", std::to_string(disabled));
  summary.add("routes", "maxHops", std::to_string(maxHops));
}

void CatalogAuditor::summarizePrices(AuditSummary &summary) const {
  const std::vector<PricePoint> prices = oracles_.prices();
  summary.add("prices", "count", std::to_string(prices.size()));
  std::uint64_t confident = 0U;
  std::uint64_t soft = 0U;
  for (const PricePoint &price : prices) {
    if (price.confidenceBps <= 25U) {
      ++confident;
    } else {
      ++soft;
    }
  }
  summary.add("prices", "tightConfidence", std::to_string(confident));
  summary.add("prices", "softConfidence", std::to_string(soft));
}

void CatalogAuditor::summarizeFees(AuditSummary &summary) const {
  summary.add("fees", "defaultBps", std::to_string(fees_.defaultBps()));
  summary.add("fees", "routeBps", std::to_string(fees_.routeBps()));
  summary.add("fees", "windowBps", std::to_string(fees_.windowBps()));
  summary.add("fees", "feeAccount", fees_.feeAccount());
}

LedgerAuditor::LedgerAuditor(const AssetRegistry &assets, const OracleBook &oracles, const Ledger &ledger)
    : assets_(assets), oracles_(oracles), ledger_(ledger) {}

AuditSummary LedgerAuditor::summarize() const {
  AuditSummary summary;
  summarizeAccounts(summary);
  summarizeAssetTotals(summary);
  return summary;
}

void LedgerAuditor::summarizeAccounts(AuditSummary &summary) const {
  std::map<std::string, bool> accounts;
  for (const BalanceRow &row : ledger_.balances()) {
    accounts[row.account] = true;
  }
  summary.add("ledger", "accounts", std::to_string(accounts.size()));
  long double totalNotional = 0.0L;
  for (const BalanceRow &row : ledger_.balances()) {
    if (!assets_.has(row.asset) || !oracles_.hasPrice(row.asset)) {
      continue;
    }
    totalNotional += notionalForBalance(row.amount, assets_.get(row.asset), oracles_.numericPrice(row.asset));
  }
  summary.add("ledger", "notional", OracleBook::formatPrice(totalNotional));
}

void LedgerAuditor::summarizeAssetTotals(AuditSummary &summary) const {
  std::map<std::string, Amount> totals;
  for (const BalanceRow &row : ledger_.balances()) {
    const Amount current = totals[row.asset];
    totals[row.asset] = current.checkedAdd(row.amount, "audit asset total");
  }
  for (const auto &entry : totals) {
    summary.add("ledger.asset", entry.first, entry.second.str());
  }
}

QuoteAuditor::QuoteAuditor(std::vector<QuoteResult> quotes) : quotes_(std::move(quotes)) {}

AuditSummary QuoteAuditor::summarize() const {
  AuditSummary summary;
  summarizeAcceptance(summary);
  summarizeRoutes(summary);
  return summary;
}

void QuoteAuditor::summarizeAcceptance(AuditSummary &summary) const {
  std::uint64_t accepted = 0U;
  std::uint64_t rejected = 0U;
  for (const QuoteResult &quote : quotes_) {
    if (quote.accepted) {
      ++accepted;
    } else {
      ++rejected;
    }
  }
  summary.add("quotes", "accepted", std::to_string(accepted));
  summary.add("quotes", "rejected", std::to_string(rejected));
}

void QuoteAuditor::summarizeRoutes(AuditSummary &summary) const {
  std::map<std::string, std::uint64_t> counts;
  for (const QuoteResult &quote : quotes_) {
    counts[quote.routeId] += 1U;
  }
  for (const auto &entry : counts) {
    summary.add("quotes.route", entry.first, std::to_string(entry.second));
  }
}

std::string auditItemsAsText(const AuditSummary &summary) {
  std::ostringstream out;
  for (const AuditItem &item : summary.items) {
    out << item.section << "." << item.key << "=" << item.value << "\n";
  }
  for (const RiskFinding &finding : summary.findings) {
    out << "finding." << riskSeverityText(finding.severity) << "." << finding.code << "=" << finding.subject << "\n";
  }
  return out.str();
}

} // namespace aurora
