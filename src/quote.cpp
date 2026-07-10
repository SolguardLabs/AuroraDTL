#include "quote.hpp"

namespace aurora {

QuoteEngine::QuoteEngine(
    const AssetRegistry &assets,
    const OracleBook &oracles,
    const RouteBook &routes,
    const FeeSchedule &fees,
    const Clock &clock)
    : assets_(assets), oracles_(oracles), routes_(routes), fees_(fees), clock_(clock) {}

QuoteResult QuoteEngine::quote(const QuoteRequest &request) const {
  require(isIdentifier(request.id), "quote request id is invalid: " + request.id);
  require(isIdentifier(request.account), "quote account id is invalid: " + request.account);
  const Route &route = routes_.get(request.routeId);
  require(!route.hops.empty(), "route has no hops: " + request.routeId);
  const std::string &sourceSymbol = route.hops.front();
  const std::string &destinationSymbol = route.hops.back();
  assets_.validateTradable(sourceSymbol, request.amountIn);
  assets_.validateSettlement(destinationSymbol);

  QuoteResult result;
  result.id = request.id;
  result.account = request.account;
  result.routeId = request.routeId;
  result.sourceAsset = sourceSymbol;
  result.destinationAsset = destinationSymbol;
  result.amountIn = request.amountIn;
  result.minOut = request.minOut;
  result.window = request.window == 0U ? route.settlementWindow : request.window;

  try {
    result.legs = routes_.simulate(route, request.amountIn, assets_, oracles_, clock_);
    if (result.legs.empty()) {
      result.accepted = false;
      result.reason = "empty-route";
      return result;
    }
    const RouteLegQuote &lastLeg = result.legs.back();
    result.grossOut = lastLeg.output;
    result.indexOut = lastLeg.normalized18;
    result.fees = fees_.quote(result.grossOut, destinationSymbol, route.feeBps, 0U);
    result.netOut = result.fees.net;
    const std::uint32_t assetTolerance = assets_.toleranceFor(destinationSymbol, route.toleranceBps);
    result.toleranceBps = request.clientToleranceBps == 0U ? assetTolerance : std::min(assetTolerance, request.clientToleranceBps);
    result.toleranceFloor = result.netOut.reduceBps(result.toleranceBps);

    if (request.minOut > result.netOut) {
      result.accepted = false;
      result.reason = "min-output";
      return result;
    }
    if (!request.minOut.isZero() && request.minOut < result.toleranceFloor) {
      result.accepted = false;
      result.reason = "tolerance-floor";
      return result;
    }
    result.accepted = true;
    result.reason = "accepted";
    return result;
  } catch (const Error &error) {
    result.accepted = false;
    result.reason = error.what();
    return result;
  }
}

std::vector<QuoteResult> QuoteEngine::quoteAll(const std::vector<QuoteRequest> &requests) const {
  std::vector<QuoteResult> results;
  results.reserve(requests.size());
  for (const QuoteRequest &request : requests) {
    results.push_back(quote(request));
  }
  return results;
}

} // namespace aurora
