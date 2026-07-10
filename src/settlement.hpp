#pragma once

#include "quote.hpp"

#include <string>
#include <vector>

namespace aurora {

struct SettlementEvent {
  std::string type;
  std::string id;
  std::string account;
  std::string asset;
  Amount amount{Amount::zero()};
  std::string note;
};

struct SettlementResult {
  std::string id;
  std::string status;
  std::string reason;
  QuoteResult quote;
  Amount sourceDebit{Amount::zero()};
  Amount destinationCredit{Amount::zero()};
  Amount destinationFee{Amount::zero()};
  Amount settlementGross{Amount::zero()};
  std::string feeAccount;
  std::uint64_t window{0};
};

class SettlementEngine {
public:
  SettlementEngine(
      const AssetRegistry &assets,
      const OracleBook &oracles,
      const RouteBook &routes,
      const FeeSchedule &fees,
      const Clock &clock,
      Ledger &ledger);

  SettlementResult settle(const QuoteRequest &request);
  std::vector<SettlementResult> settleAll(const std::vector<QuoteRequest> &requests);
  const std::vector<SettlementEvent> &events() const;

private:
  const AssetRegistry &assets_;
  const OracleBook &oracles_;
  const RouteBook &routes_;
  const FeeSchedule &fees_;
  const Clock &clock_;
  Ledger &ledger_;
  std::vector<SettlementEvent> events_;
};

} // namespace aurora
