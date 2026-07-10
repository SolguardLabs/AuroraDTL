#pragma once

#include "model.hpp"

#include <string>
#include <vector>

namespace aurora {

struct RunReport {
  std::string scenario;
  Clock clock;
  std::vector<OracleDecision> oracleDecisions;
  std::vector<QuoteResult> quotes;
  std::vector<SettlementResult> settlements;
  std::vector<SettlementEvent> settlementEvents;
  std::vector<LedgerEvent> ledgerEvents;
  std::vector<BalanceRow> balances;
};

std::string writeRunReport(const RunReport &report, bool includeEvents);
std::string writeValidationReport(const Scenario &scenario);
void writeAmount(JsonWriter &writer, const std::string &key, Amount amount);
void writePrice(JsonWriter &writer, const std::string &key, long double price);

} // namespace aurora
