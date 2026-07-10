#pragma once

#include "ledger.hpp"
#include "quote.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

enum class RiskSeverity { Info, Warning, Blocking };

std::string riskSeverityText(RiskSeverity severity);

struct RiskFinding {
  RiskSeverity severity{RiskSeverity::Info};
  std::string code;
  std::string subject;
  std::string detail;
};

struct RouteRiskMetrics {
  std::string routeId;
  std::string pair;
  std::uint64_t hopCount{0};
  std::uint32_t maxAssetToleranceBps{0};
  std::uint32_t routeToleranceBps{0};
  std::uint32_t routeFeeBps{0};
  bool enabled{true};
  bool allPricesAvailable{true};
};

struct AccountExposure {
  std::string account;
  std::map<std::string, Amount> balances;
  long double notional{0.0L};
};

struct RiskConfig {
  std::uint64_t maxRouteHops{6};
  std::uint32_t maxCombinedFeeBps{250};
  std::uint32_t maxToleranceBps{150};
  long double maxAccountNotional{0.0L};
};

struct RiskReport {
  std::vector<RouteRiskMetrics> routes;
  std::vector<AccountExposure> accounts;
  std::vector<RiskFinding> findings;

  bool hasBlockingFindings() const;
  std::vector<RiskFinding> bySeverity(RiskSeverity severity) const;
};

class RiskEngine {
public:
  RiskEngine(
      const AssetRegistry &assets,
      const OracleBook &oracles,
      const RouteBook &routes,
      const FeeSchedule &fees,
      const Ledger &ledger,
      RiskConfig config);

  RiskReport evaluate() const;
  RouteRiskMetrics routeMetrics(const Route &route) const;
  AccountExposure accountExposure(const std::string &account, const std::map<std::string, Amount> &balances) const;
  std::vector<RiskFinding> routeFindings(const RouteRiskMetrics &metrics) const;
  std::vector<RiskFinding> accountFindings(const AccountExposure &exposure) const;

private:
  const AssetRegistry &assets_;
  const OracleBook &oracles_;
  const RouteBook &routes_;
  const FeeSchedule &fees_;
  const Ledger &ledger_;
  RiskConfig config_;
};

long double notionalForBalance(Amount amount, const Asset &asset, long double price);

} // namespace aurora
