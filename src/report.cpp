#include "report.hpp"

namespace aurora {

namespace {

void writeClock(JsonWriter &writer, const Clock &clock) {
  writer.key("clock");
  writer.beginObject();
  writer.key("timestamp");
  writer.numberValue(clock.timestamp);
  writer.key("window");
  writer.numberValue(clock.window);
  writer.endObject();
}

void writeFeeBreakdown(JsonWriter &writer, const FeeBreakdown &fees) {
  writer.key("fees");
  writer.beginObject();
  writer.key("baseBps");
  writer.numberValue(fees.baseBps);
  writer.key("routeBps");
  writer.numberValue(fees.routeBps);
  writer.key("windowBps");
  writer.numberValue(fees.windowBps);
  writer.key("assetBps");
  writer.numberValue(fees.assetBps);
  writer.key("totalBps");
  writer.numberValue(fees.totalBps);
  writeAmount(writer, "fee", fees.fee);
  writeAmount(writer, "net", fees.net);
  writer.endObject();
}

void writeLeg(JsonWriter &writer, const RouteLegQuote &leg) {
  writer.beginObject();
  writer.key("source");
  writer.stringValue(leg.source);
  writer.key("destination");
  writer.stringValue(leg.destination);
  writeAmount(writer, "input", leg.input);
  writeAmount(writer, "normalized18", leg.normalized18);
  writeAmount(writer, "output", leg.output);
  writePrice(writer, "sourcePrice", leg.sourcePrice);
  writePrice(writer, "destinationPrice", leg.destinationPrice);
  writer.endObject();
}

void writeQuote(JsonWriter &writer, const QuoteResult &quote) {
  writer.beginObject();
  writer.key("id");
  writer.stringValue(quote.id);
  writer.key("account");
  writer.stringValue(quote.account);
  writer.key("route");
  writer.stringValue(quote.routeId);
  writer.key("source");
  writer.stringValue(quote.sourceAsset);
  writer.key("destination");
  writer.stringValue(quote.destinationAsset);
  writeAmount(writer, "amountIn", quote.amountIn);
  writeAmount(writer, "grossOut", quote.grossOut);
  writeAmount(writer, "netOut", quote.netOut);
  writeAmount(writer, "minOut", quote.minOut);
  writeAmount(writer, "toleranceFloor", quote.toleranceFloor);
  writeAmount(writer, "indexOut", quote.indexOut);
  writer.key("toleranceBps");
  writer.numberValue(quote.toleranceBps);
  writer.key("window");
  writer.numberValue(quote.window);
  writer.key("accepted");
  writer.boolValue(quote.accepted);
  writer.key("reason");
  writer.stringValue(quote.reason);
  writeFeeBreakdown(writer, quote.fees);
  writer.key("legs");
  writer.beginArray();
  for (const RouteLegQuote &leg : quote.legs) {
    writeLeg(writer, leg);
  }
  writer.endArray();
  writer.endObject();
}

void writeSettlement(JsonWriter &writer, const SettlementResult &settlement) {
  writer.beginObject();
  writer.key("id");
  writer.stringValue(settlement.id);
  writer.key("status");
  writer.stringValue(settlement.status);
  writer.key("reason");
  writer.stringValue(settlement.reason);
  writer.key("feeAccount");
  writer.stringValue(settlement.feeAccount);
  writer.key("window");
  writer.numberValue(settlement.window);
  writeAmount(writer, "sourceDebit", settlement.sourceDebit);
  writeAmount(writer, "settlementGross", settlement.settlementGross);
  writeAmount(writer, "destinationCredit", settlement.destinationCredit);
  writeAmount(writer, "destinationFee", settlement.destinationFee);
  writer.key("quote");
  writeQuote(writer, settlement.quote);
  writer.endObject();
}

void writeOracleDecision(JsonWriter &writer, const OracleDecision &decision) {
  writer.beginObject();
  writer.key("asset");
  writer.stringValue(decision.asset);
  writer.key("accepted");
  writer.boolValue(decision.accepted);
  writer.key("reason");
  writer.stringValue(decision.reason);
  writer.key("deviationBps");
  writer.numberValue(decision.deviationBps);
  writer.key("previousSequence");
  writer.numberValue(decision.previousSequence);
  writer.key("nextSequence");
  writer.numberValue(decision.nextSequence);
  writePrice(writer, "previousPrice", decision.previousPrice);
  writePrice(writer, "nextPrice", decision.nextPrice);
  writer.endObject();
}

void writeSettlementEvent(JsonWriter &writer, const SettlementEvent &event) {
  writer.beginObject();
  writer.key("type");
  writer.stringValue(event.type);
  writer.key("id");
  writer.stringValue(event.id);
  writer.key("account");
  writer.stringValue(event.account);
  writer.key("asset");
  writer.stringValue(event.asset);
  writeAmount(writer, "amount", event.amount);
  writer.key("note");
  writer.stringValue(event.note);
  writer.endObject();
}

void writeLedgerEvent(JsonWriter &writer, const LedgerEvent &event) {
  writer.beginObject();
  writer.key("type");
  writer.stringValue(event.type);
  writer.key("account");
  writer.stringValue(event.account);
  writer.key("asset");
  writer.stringValue(event.asset);
  writeAmount(writer, "amount", event.amount);
  writer.key("ref");
  writer.stringValue(event.ref);
  writer.endObject();
}

void writeBalance(JsonWriter &writer, const BalanceRow &balance) {
  writer.beginObject();
  writer.key("account");
  writer.stringValue(balance.account);
  writer.key("asset");
  writer.stringValue(balance.asset);
  writeAmount(writer, "amount", balance.amount);
  writer.endObject();
}

} // namespace

std::string writeRunReport(const RunReport &report, bool includeEvents) {
  JsonWriter writer;
  writer.beginObject();
  writer.key("scenario");
  writer.stringValue(report.scenario);
  writer.key("status");
  writer.stringValue("ok");
  writeClock(writer, report.clock);

  writer.key("oracleUpdates");
  writer.beginArray();
  for (const OracleDecision &decision : report.oracleDecisions) {
    writeOracleDecision(writer, decision);
  }
  writer.endArray();

  writer.key("quotes");
  writer.beginArray();
  for (const QuoteResult &quote : report.quotes) {
    writeQuote(writer, quote);
  }
  writer.endArray();

  writer.key("settlements");
  writer.beginArray();
  for (const SettlementResult &settlement : report.settlements) {
    writeSettlement(writer, settlement);
  }
  writer.endArray();

  writer.key("balances");
  writer.beginArray();
  for (const BalanceRow &balance : report.balances) {
    writeBalance(writer, balance);
  }
  writer.endArray();

  if (includeEvents) {
    writer.key("events");
    writer.beginObject();
    writer.key("settlement");
    writer.beginArray();
    for (const SettlementEvent &event : report.settlementEvents) {
      writeSettlementEvent(writer, event);
    }
    writer.endArray();
    writer.key("ledger");
    writer.beginArray();
    for (const LedgerEvent &event : report.ledgerEvents) {
      writeLedgerEvent(writer, event);
    }
    writer.endArray();
    writer.endObject();
  }

  writer.endObject();
  return writer.str();
}

std::string writeValidationReport(const Scenario &scenario) {
  JsonWriter writer;
  writer.beginObject();
  writer.key("scenario");
  writer.stringValue(scenario.name);
  writer.key("status");
  writer.stringValue("ok");
  writer.key("assets");
  writer.numberValue(static_cast<std::uint64_t>(scenario.assets.all().size()));
  writer.key("routes");
  writer.numberValue(static_cast<std::uint64_t>(scenario.routes.all().size()));
  writer.key("orders");
  writer.numberValue(static_cast<std::uint64_t>(scenario.orders.size()));
  writer.endObject();
  return writer.str();
}

void writeAmount(JsonWriter &writer, const std::string &key, Amount amount) {
  writer.key(key);
  writer.stringValue(amount.str());
}

void writePrice(JsonWriter &writer, const std::string &key, long double price) {
  writer.key(key);
  writer.stringValue(OracleBook::formatPrice(price));
}

} // namespace aurora
