#include "stress.hpp"

#include "risk.hpp"

#include <algorithm>
#include <cmath>
#include <map>

namespace aurora {

namespace {

std::uint32_t boundedBps(long double value) {
  if (!std::isfinite(static_cast<double>(value)) || value <= 0.0L) {
    return 0U;
  }
  if (value >= 10000.0L) {
    return 10000U;
  }
  return static_cast<std::uint32_t>(std::floor(value + 0.5L));
}

std::uint32_t effectiveShock(const StressScenario &scenario, std::uint32_t confidenceBps) {
  const long double confidenceAddOn =
      static_cast<long double>(confidenceBps) * static_cast<long double>(scenario.confidenceMultiplierBps) / 10000.0L;
  const long double combined = static_cast<long double>(scenario.marketShockBps) + confidenceAddOn;
  return boundedBps(combined);
}

} // namespace

bool StressReport::requiresIntervention() const {
  return severity == "high" || severity == "critical";
}

long double StressReport::reserveRequiredFor(std::uint32_t targetCoverageBps) const {
  require(targetCoverageBps <= 10000U, "target coverage bps out of range");
  return lossNotional * static_cast<long double>(targetCoverageBps) / 10000.0L;
}

PortfolioStressEngine::PortfolioStressEngine(
    const AssetRegistry &assets,
    const OracleBook &oracles,
    const Ledger &ledger)
    : assets_(assets), oracles_(oracles), ledger_(ledger) {}

StressReport PortfolioStressEngine::evaluate(const StressScenario &scenario) const {
  require(scenario.marketShockBps <= 10000U, "market shock bps out of range");
  require(scenario.confidenceMultiplierBps <= 10000U, "confidence multiplier bps out of range");
  require(scenario.accessibilityHaircutBps <= 10000U, "accessibility haircut bps out of range");
  require(scenario.reserveBufferNotional >= 0.0L, "reserve buffer must be non-negative");

  std::map<std::string, Amount> totals;
  for (const BalanceRow &row : ledger_.balances()) {
    totals[row.asset] = totals[row.asset].checkedAdd(row.amount, "stress asset balance");
  }

  StressReport report;
  report.scenario = scenario.name;
  report.reserveBufferNotional = scenario.reserveBufferNotional;

  for (const auto &entry : totals) {
    if (!assets_.has(entry.first) || !oracles_.hasPrice(entry.first)) {
      continue;
    }
    const Asset &asset = assets_.get(entry.first);
    const PricePoint &price = oracles_.priceOf(entry.first);
    StressAssetExposure exposure;
    exposure.asset = entry.first;
    exposure.confidenceBps = price.confidenceBps;
    exposure.effectiveShockBps = effectiveShock(scenario, price.confidenceBps);
    exposure.baselineNotional = notionalForBalance(entry.second, asset, price.price);

    const long double priceRetention =
        static_cast<long double>(10000U - exposure.effectiveShockBps) / 10000.0L;
    const long double accessibilityRetention =
        static_cast<long double>(10000U - scenario.accessibilityHaircutBps) / 10000.0L;
    exposure.shockedNotional = exposure.baselineNotional * priceRetention * accessibilityRetention;
    report.baselineNotional += exposure.baselineNotional;
    report.shockedNotional += exposure.shockedNotional;
    report.assets.push_back(exposure);
  }

  report.lossNotional = std::max(0.0L, report.baselineNotional - report.shockedNotional);
  report.uncoveredLossNotional = std::max(0.0L, report.lossNotional - report.reserveBufferNotional);
  if (report.baselineNotional > 0.0L) {
    report.lossBps = boundedBps(report.lossNotional * 10000.0L / report.baselineNotional);
  }
  if (report.lossNotional > 0.0L) {
    report.reserveCoverageBps = boundedBps(report.reserveBufferNotional * 10000.0L / report.lossNotional);
  } else {
    report.reserveCoverageBps = 10000U;
  }

  long double hhiAccumulator = 0.0L;
  for (StressAssetExposure &exposure : report.assets) {
    if (report.baselineNotional > 0.0L) {
      exposure.portfolioShareBps =
          boundedBps(exposure.baselineNotional * 10000.0L / report.baselineNotional);
    }
    report.largestAssetShareBps = std::max(report.largestAssetShareBps, exposure.portfolioShareBps);
    const long double share = static_cast<long double>(exposure.portfolioShareBps) / 10000.0L;
    hhiAccumulator += share * share;
  }
  report.concentrationHhiBps = boundedBps(hhiAccumulator * 10000.0L);
  report.severity = stressSeverity(report.lossBps, report.concentrationHhiBps, report.uncoveredLossNotional);
  return report;
}

std::string stressSeverity(
    std::uint32_t lossBps,
    std::uint32_t concentrationHhiBps,
    long double uncoveredLoss) {
  if (lossBps >= 2500U && uncoveredLoss > 0.0L) {
    return "critical";
  }
  if ((lossBps >= 1500U && uncoveredLoss > 0.0L) || concentrationHhiBps >= 5000U) {
    return "high";
  }
  if (lossBps >= 750U || concentrationHhiBps >= 2500U) {
    return "moderate";
  }
  return "low";
}

} // namespace aurora
