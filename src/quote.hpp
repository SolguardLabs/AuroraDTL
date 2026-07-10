#pragma once

#include "fees.hpp"
#include "ledger.hpp"
#include "route.hpp"

#include <string>
#include <vector>

namespace aurora {

struct QuoteRequest {
  std::string id;
  std::string account;
  std::string routeId;
  Amount amountIn{Amount::zero()};
  Amount minOut{Amount::zero()};
  std::uint32_t clientToleranceBps{0};
  std::uint64_t window{0};
  bool settle{true};
  std::string memo;
};

struct QuoteResult {
  std::string id;
  std::string account;
  std::string routeId;
  std::string sourceAsset;
  std::string destinationAsset;
  Amount amountIn{Amount::zero()};
  Amount grossOut{Amount::zero()};
  Amount netOut{Amount::zero()};
  Amount minOut{Amount::zero()};
  Amount indexOut{Amount::zero()};
  Amount toleranceFloor{Amount::zero()};
  FeeBreakdown fees;
  std::uint32_t toleranceBps{0};
  std::uint64_t window{0};
  bool accepted{false};
  std::string reason;
  std::vector<RouteLegQuote> legs;
};

class QuoteEngine {
public:
  QuoteEngine(const AssetRegistry &assets, const OracleBook &oracles, const RouteBook &routes, const FeeSchedule &fees, const Clock &clock);

  QuoteResult quote(const QuoteRequest &request) const;
  std::vector<QuoteResult> quoteAll(const std::vector<QuoteRequest> &requests) const;

private:
  const AssetRegistry &assets_;
  const OracleBook &oracles_;
  const RouteBook &routes_;
  const FeeSchedule &fees_;
  const Clock &clock_;
};

} // namespace aurora
