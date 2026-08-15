#pragma once

#include "ledger.hpp"
#include "oracle.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace aurora {

struct StressScenario {
  std::string name{"correlated-downside"};
  std::uint32_t marketShockBps{1500};
  std::uint32_t confidenceMultiplierBps{2500};
  std::uint32_t accessibilityHaircutBps{500};
  long double reserveBufferNotional{0.0L};
};

struct StressAssetExposure {
  std::string asset;
  long double baselineNotional{0.0L};
  long double shockedNotional{0.0L};
  std::uint32_t effectiveShockBps{0};
  std::uint32_t portfolioShareBps{0};
  std::uint32_t confidenceBps{0};
};

struct StressReport {
  std::string scenario;
  long double baselineNotional{0.0L};
  long double shockedNotional{0.0L};
  long double lossNotional{0.0L};
  long double reserveBufferNotional{0.0L};
  long double uncoveredLossNotional{0.0L};
  std::uint32_t concentrationHhiBps{0};
  std::uint32_t largestAssetShareBps{0};
  std::uint32_t lossBps{0};
  std::uint32_t reserveCoverageBps{0};
  std::string severity;
  std::vector<StressAssetExposure> assets;

  bool requiresIntervention() const;
  long double reserveRequiredFor(std::uint32_t targetCoverageBps) const;
};

class PortfolioStressEngine {
public:
  PortfolioStressEngine(const AssetRegistry &assets, const OracleBook &oracles, const Ledger &ledger);

  StressReport evaluate(const StressScenario &scenario) const;

private:
  const AssetRegistry &assets_;
  const OracleBook &oracles_;
  const Ledger &ledger_;
};

std::string stressSeverity(std::uint32_t lossBps, std::uint32_t concentrationHhiBps, long double uncoveredLoss);

} // namespace aurora
