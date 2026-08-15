#include "checkpoint.hpp"
#include "stress.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

template <typename Callback> void expectAuroraError(Callback callback, const std::string &message) {
  try {
    callback();
  } catch (const aurora::Error &) {
    return;
  }
  throw std::runtime_error(message);
}

aurora::Asset makeAsset(const std::string &symbol, std::uint8_t decimals) {
  aurora::Asset asset;
  asset.symbol = symbol;
  asset.displayName = symbol;
  asset.decimals = decimals;
  asset.status = aurora::AssetStatus::Active;
  asset.maxDeviationBps = 200U;
  asset.defaultToleranceBps = 100U;
  asset.settlementEnabled = true;
  return asset;
}

aurora::PricePoint makePrice(const std::string &asset, long double price, std::uint32_t confidenceBps) {
  aurora::PricePoint point;
  point.asset = asset;
  point.price = price;
  point.confidenceBps = confidenceBps;
  point.updatedAt = 1800000000U;
  point.sequence = 1U;
  point.source = "committee";
  return point;
}

void testStressModel() {
  aurora::AssetRegistry assets;
  assets.add(makeAsset("WETH18", 18U));
  assets.add(makeAsset("USDC6", 6U));

  aurora::OracleBook oracles;
  oracles.seed(makePrice("WETH18", 3000.0L, 40U), assets);
  oracles.seed(makePrice("USDC6", 1.0L, 5U), assets);

  aurora::Ledger ledger;
  ledger.credit("maker", "WETH18", aurora::Amount::fromString("2000000000000000000", "weth"), "seed");
  ledger.credit("maker", "USDC6", aurora::Amount::fromString("7000000000", "usdc"), "seed");

  aurora::StressScenario scenario;
  scenario.marketShockBps = 2000U;
  scenario.confidenceMultiplierBps = 5000U;
  scenario.accessibilityHaircutBps = 1000U;
  scenario.reserveBufferNotional = 1500.0L;

  aurora::PortfolioStressEngine engine(assets, oracles, ledger);
  const aurora::StressReport report = engine.evaluate(scenario);
  check(report.assets.size() == 2U, "stress report must include both assets");
  check(report.baselineNotional > 12999.0L && report.baselineNotional < 13001.0L, "unexpected baseline notional");
  check(report.shockedNotional < report.baselineNotional, "shock must reduce notional");
  check(report.lossNotional > report.reserveBufferNotional, "scenario must expose an uncovered loss");
  check(report.reserveCoverageBps > 0U && report.reserveCoverageBps < 10000U, "reserve coverage must be bounded");
  check(report.largestAssetShareBps > 5000U, "stable inventory should be the largest exposure");
  check(report.reserveRequiredFor(8000U) > scenario.reserveBufferNotional, "target reserve must exceed current buffer");
}

aurora::CheckpointPayload checkpointPayload(std::uint64_t sequence, const std::string &previousDigest) {
  aurora::CheckpointPayload payload;
  payload.sequence = sequence;
  payload.window = 30000001U + sequence;
  payload.previousDigest = previousDigest;
  payload.scenario = "native-operations";
  payload.balances = {
      aurora::BalanceRow{"alice", "USD18", aurora::Amount(500U)},
      aurora::BalanceRow{"treasury", "EUR18", aurora::Amount(25U)},
  };
  payload.events = {
      aurora::SettlementEvent{"credit", "order-1", "alice", "EUR18", aurora::Amount(100U), "destination"},
  };
  return payload;
}

void testCheckpointChain() {
  aurora::CheckpointPayload firstPayload = checkpointPayload(1U, "");
  aurora::CheckpointPayload reordered = firstPayload;
  std::reverse(reordered.balances.begin(), reordered.balances.end());
  const auto firstDigest = aurora::CheckpointBuilder::build(firstPayload).digest;
  const auto reorderedDigest = aurora::CheckpointBuilder::build(reordered).digest;
  check(firstDigest == reorderedDigest, "checkpoint digest must be balance-order independent");
  check(firstDigest.size() == 64U, "checkpoint digest must contain 256 bits of hexadecimal data");

  aurora::CheckpointRegistry registry(2U);
  registry.addReviewer("operations");
  registry.addReviewer("risk");
  registry.addReviewer("finance");

  const auto first = registry.propose("operations", firstPayload);
  check(registry.hasPending(), "first checkpoint must wait for quorum");
  check(registry.approve("risk", first.digest), "second approval must finalize checkpoint");
  check(registry.finalized().size() == 1U, "one checkpoint must be finalized");

  const auto second = registry.propose("risk", checkpointPayload(2U, registry.latestDigest()));
  expectAuroraError(
      [&]() { registry.approve("risk", second.digest); },
      "duplicate reviewer approval must be rejected");
  check(registry.approve("finance", second.digest), "independent approval must finalize second checkpoint");
  check(registry.finalized().size() == 2U, "checkpoint chain must advance sequentially");

  expectAuroraError(
      [&]() { registry.propose("operations", checkpointPayload(4U, registry.latestDigest())); },
      "non-contiguous checkpoint sequence must be rejected");
  expectAuroraError(
      [&]() { registry.removeReviewer("operations"); registry.removeReviewer("risk"); },
      "reviewer removal must preserve quorum");
}

} // namespace

int main() {
  try {
    testStressModel();
    testCheckpointChain();
    std::cout << "native operations tests: 2 passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "native operations tests failed: " << error.what() << "\n";
    return 1;
  }
}
