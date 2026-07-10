#include "settlement.hpp"

namespace aurora {

SettlementEngine::SettlementEngine(
    const AssetRegistry &assets,
    const OracleBook &oracles,
    const RouteBook &routes,
    const FeeSchedule &fees,
    const Clock &clock,
    Ledger &ledger)
    : assets_(assets), oracles_(oracles), routes_(routes), fees_(fees), clock_(clock), ledger_(ledger) {}

SettlementResult SettlementEngine::settle(const QuoteRequest &request) {
  QuoteEngine quoteEngine(assets_, oracles_, routes_, fees_, clock_);
  SettlementResult result;
  result.id = request.id;
  result.quote = quoteEngine.quote(request);
  result.window = result.quote.window;
  result.feeAccount = fees_.feeAccount();

  if (!request.settle) {
    result.status = "quoted";
    result.reason = result.quote.reason;
    events_.push_back(SettlementEvent{"quote", request.id, request.account, result.quote.destinationAsset, result.quote.netOut, result.reason});
    return result;
  }

  if (!result.quote.accepted) {
    result.status = "rejected";
    result.reason = result.quote.reason;
    events_.push_back(SettlementEvent{"reject", request.id, request.account, result.quote.destinationAsset, Amount::zero(), result.reason});
    return result;
  }

  try {
    const std::string &sourceAsset = result.quote.sourceAsset;
    const std::string &destinationAsset = result.quote.destinationAsset;
    ledger_.debit(request.account, sourceAsset, result.quote.amountIn, request.id);

    const Route &route = routes_.get(request.routeId);
    result.settlementGross = result.quote.indexOut;
    const FeeBreakdown executionFees = fees_.quote(result.settlementGross, destinationAsset, route.feeBps, 0U);
    result.destinationFee = executionFees.fee;
    result.destinationCredit = executionFees.net;

    ledger_.credit(request.account, destinationAsset, result.destinationCredit, request.id);
    if (!result.destinationFee.isZero()) {
      ledger_.credit(fees_.feeAccount(), destinationAsset, result.destinationFee, request.id + ".fee");
    }
    result.sourceDebit = result.quote.amountIn;
    result.status = "settled";
    result.reason = "accepted";
    events_.push_back(SettlementEvent{"debit", request.id, request.account, sourceAsset, result.sourceDebit, "source"});
    events_.push_back(SettlementEvent{"credit", request.id, request.account, destinationAsset, result.destinationCredit, "destination"});
    if (!result.destinationFee.isZero()) {
      events_.push_back(SettlementEvent{"fee", request.id, fees_.feeAccount(), destinationAsset, result.destinationFee, "route"});
    }
  } catch (const Error &error) {
    result.status = "failed";
    result.reason = error.what();
    events_.push_back(SettlementEvent{"fail", request.id, request.account, result.quote.destinationAsset, Amount::zero(), result.reason});
  }
  return result;
}

std::vector<SettlementResult> SettlementEngine::settleAll(const std::vector<QuoteRequest> &requests) {
  std::vector<SettlementResult> results;
  results.reserve(requests.size());
  for (const QuoteRequest &request : requests) {
    results.push_back(settle(request));
  }
  return results;
}

const std::vector<SettlementEvent> &SettlementEngine::events() const {
  return events_;
}

} // namespace aurora
