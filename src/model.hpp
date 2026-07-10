#pragma once

#include "json.hpp"
#include "settlement.hpp"

#include <string>
#include <vector>

namespace aurora {

struct Scenario {
  std::string name;
  Clock clock;
  AssetRegistry assets;
  OracleBook oracles;
  FeeSchedule fees;
  RouteBook routes;
  Ledger ledger;
  std::vector<PriceUpdate> priceUpdates;
  std::vector<QuoteRequest> orders;
};

Scenario loadScenario(const std::string &path);
void validateScenario(const Scenario &scenario);

Asset parseAsset(const JsonValue &value);
PricePoint parsePricePoint(const JsonValue &value);
PriceUpdate parsePriceUpdate(const JsonValue &value);
Route parseRoute(const JsonValue &value);
QuoteRequest parseOrder(const JsonValue &value);
FeeSchedule parseFees(const JsonValue &root);
Ledger parseLedger(const JsonValue &root);
Clock parseClock(const JsonValue &root);

} // namespace aurora
