#include "model.hpp"
#include "report.hpp"

#include <iostream>

namespace aurora {

namespace {

struct CliOptions {
  std::string command;
  std::string fixture;
  bool json{false};
  bool events{false};
};

void printUsage() {
  std::cerr << "usage: auroradtl <validate|quote|run|settle> <fixture> [--json] [--events]\n";
}

CliOptions parseCli(int argc, char **argv) {
  if (argc < 3) {
    printUsage();
    throw ValidationError("missing command or fixture");
  }
  CliOptions options;
  options.command = argv[1];
  options.fixture = argv[2];
  for (int i = 3; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json") {
      options.json = true;
    } else if (arg == "--events") {
      options.events = true;
    } else {
      throw ValidationError("unknown cli option: " + arg);
    }
  }
  return options;
}

std::vector<OracleDecision> applyUpdates(Scenario &scenario) {
  std::vector<OracleDecision> decisions;
  decisions.reserve(scenario.priceUpdates.size());
  for (const PriceUpdate &update : scenario.priceUpdates) {
    decisions.push_back(scenario.oracles.apply(update, scenario.assets, scenario.clock));
  }
  return decisions;
}

RunReport quoteScenario(Scenario &scenario) {
  RunReport report;
  report.scenario = scenario.name;
  report.clock = scenario.clock;
  report.oracleDecisions = applyUpdates(scenario);
  QuoteEngine engine(scenario.assets, scenario.oracles, scenario.routes, scenario.fees, scenario.clock);
  report.quotes = engine.quoteAll(scenario.orders);
  report.balances = scenario.ledger.balances();
  return report;
}

RunReport settleScenario(Scenario &scenario) {
  RunReport report;
  report.scenario = scenario.name;
  report.clock = scenario.clock;
  report.oracleDecisions = applyUpdates(scenario);
  SettlementEngine engine(scenario.assets, scenario.oracles, scenario.routes, scenario.fees, scenario.clock, scenario.ledger);
  report.settlements = engine.settleAll(scenario.orders);
  report.quotes.reserve(report.settlements.size());
  for (const SettlementResult &settlement : report.settlements) {
    report.quotes.push_back(settlement.quote);
  }
  report.settlementEvents = engine.events();
  report.ledgerEvents = scenario.ledger.events();
  report.balances = scenario.ledger.balances();
  return report;
}

void printTextSummary(const RunReport &report) {
  std::cout << "scenario=" << report.scenario << "\n";
  std::cout << "quotes=" << report.quotes.size() << "\n";
  std::cout << "settlements=" << report.settlements.size() << "\n";
  for (const QuoteResult &quote : report.quotes) {
    std::cout << quote.id << " " << quote.routeId << " " << quote.reason << " net=" << quote.netOut.str() << "\n";
  }
}

} // namespace

int run(int argc, char **argv) {
  const CliOptions options = parseCli(argc, argv);
  Scenario scenario = loadScenario(options.fixture);

  if (options.command == "validate") {
    if (options.json) {
      std::cout << writeValidationReport(scenario) << "\n";
    } else {
      std::cout << "ok\n";
    }
    return 0;
  }

  if (options.command == "quote") {
    RunReport report = quoteScenario(scenario);
    if (options.json) {
      std::cout << writeRunReport(report, options.events) << "\n";
    } else {
      printTextSummary(report);
    }
    return 0;
  }

  if (options.command == "run" || options.command == "settle") {
    RunReport report = settleScenario(scenario);
    if (options.json) {
      std::cout << writeRunReport(report, options.events) << "\n";
    } else {
      printTextSummary(report);
    }
    return 0;
  }

  printUsage();
  throw ValidationError("unknown command: " + options.command);
}

} // namespace aurora

int main(int argc, char **argv) {
  try {
    return aurora::run(argc, argv);
  } catch (const std::exception &error) {
    std::cerr << "error: " << error.what() << "\n";
    return 1;
  }
}
