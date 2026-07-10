#include "risk.hpp"

namespace aurora {

std::string riskSeverityText(RiskSeverity severity) {
  switch (severity) {
  case RiskSeverity::Info:
    return "info";
  case RiskSeverity::Warning:
    return "warning";
  case RiskSeverity::Blocking:
    return "blocking";
  }
  return "unknown";
}

bool RiskReport::hasBlockingFindings() const {
  for (const RiskFinding &finding : findings) {
    if (finding.severity == RiskSeverity::Blocking) {
      return true;
    }
  }
  return false;
}

std::vector<RiskFinding> RiskReport::bySeverity(RiskSeverity severity) const {
  std::vector<RiskFinding> result;
  for (const RiskFinding &finding : findings) {
    if (finding.severity == severity) {
      result.push_back(finding);
    }
  }
  return result;
}

RiskEngine::RiskEngine(
    const AssetRegistry &assets,
    const OracleBook &oracles,
    const RouteBook &routes,
    const FeeSchedule &fees,
    const Ledger &ledger,
    RiskConfig config)
    : assets_(assets), oracles_(oracles), routes_(routes), fees_(fees), ledger_(ledger), config_(config) {}

RiskReport RiskEngine::evaluate() const {
  RiskReport report;
  for (const auto &entry : routes_.all()) {
    RouteRiskMetrics metrics = routeMetrics(entry.second);
    std::vector<RiskFinding> findings = routeFindings(metrics);
    report.routes.push_back(metrics);
    report.findings.insert(report.findings.end(), findings.begin(), findings.end());
  }

  std::map<std::string, std::map<std::string, Amount>> grouped;
  for (const BalanceRow &row : ledger_.balances()) {
    grouped[row.account][row.asset] = row.amount;
  }
  for (const auto &entry : grouped) {
    AccountExposure exposure = accountExposure(entry.first, entry.second);
    std::vector<RiskFinding> findings = accountFindings(exposure);
    report.accounts.push_back(exposure);
    report.findings.insert(report.findings.end(), findings.begin(), findings.end());
  }
  return report;
}

RouteRiskMetrics RiskEngine::routeMetrics(const Route &route) const {
  RouteRiskMetrics metrics;
  metrics.routeId = route.id;
  metrics.pair = routePair(route);
  metrics.hopCount = static_cast<std::uint64_t>(route.hops.size() > 0U ? route.hops.size() - 1U : 0U);
  metrics.routeToleranceBps = route.toleranceBps;
  metrics.routeFeeBps = route.feeBps + fees_.routeBps() + fees_.defaultBps();
  metrics.enabled = route.enabled;
  metrics.allPricesAvailable = true;
  for (const std::string &symbol : route.hops) {
    if (!assets_.has(symbol) || !oracles_.hasPrice(symbol)) {
      metrics.allPricesAvailable = false;
      continue;
    }
    const Asset &asset = assets_.get(symbol);
    metrics.maxAssetToleranceBps = std::max(metrics.maxAssetToleranceBps, asset.defaultToleranceBps);
  }
  return metrics;
}

AccountExposure RiskEngine::accountExposure(const std::string &account, const std::map<std::string, Amount> &balances) const {
  AccountExposure exposure;
  exposure.account = account;
  exposure.balances = balances;
  exposure.notional = 0.0L;
  for (const auto &entry : balances) {
    if (!assets_.has(entry.first) || !oracles_.hasPrice(entry.first)) {
      continue;
    }
    const Asset &asset = assets_.get(entry.first);
    exposure.notional += notionalForBalance(entry.second, asset, oracles_.numericPrice(entry.first));
  }
  return exposure;
}

std::vector<RiskFinding> RiskEngine::routeFindings(const RouteRiskMetrics &metrics) const {
  std::vector<RiskFinding> findings;
  if (!metrics.enabled) {
    findings.push_back(RiskFinding{RiskSeverity::Info, "route.disabled", metrics.routeId, "route is disabled"});
  }
  if (!metrics.allPricesAvailable) {
    findings.push_back(RiskFinding{RiskSeverity::Blocking, "route.price-missing", metrics.routeId, "route references asset without price"});
  }
  if (metrics.hopCount > config_.maxRouteHops) {
    findings.push_back(RiskFinding{RiskSeverity::Warning, "route.depth", metrics.routeId, "route hop count exceeds configured review threshold"});
  }
  if (metrics.routeFeeBps > config_.maxCombinedFeeBps) {
    findings.push_back(RiskFinding{RiskSeverity::Warning, "route.fee", metrics.routeId, "combined route fee exceeds configured review threshold"});
  }
  const std::uint32_t effectiveTolerance = std::max(metrics.routeToleranceBps, metrics.maxAssetToleranceBps);
  if (effectiveTolerance > config_.maxToleranceBps) {
    findings.push_back(RiskFinding{RiskSeverity::Warning, "route.tolerance", metrics.routeId, "route tolerance exceeds configured review threshold"});
  }
  return findings;
}

std::vector<RiskFinding> RiskEngine::accountFindings(const AccountExposure &exposure) const {
  std::vector<RiskFinding> findings;
  if (config_.maxAccountNotional > 0.0L && exposure.notional > config_.maxAccountNotional) {
    findings.push_back(RiskFinding{RiskSeverity::Warning, "account.notional", exposure.account, "account notional exceeds configured review threshold"});
  }
  for (const auto &entry : exposure.balances) {
    if (!assets_.has(entry.first)) {
      findings.push_back(RiskFinding{RiskSeverity::Blocking, "account.asset-missing", exposure.account, "account balance references unknown asset"});
    }
  }
  return findings;
}

long double notionalForBalance(Amount amount, const Asset &asset, long double price) {
  return toWholeUnits(amount, asset.decimals) * price;
}

} // namespace aurora
