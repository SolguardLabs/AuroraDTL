#include "model.hpp"

namespace aurora {

namespace {

Amount amountField(const JsonValue &object, const std::string &field, const std::string &context) {
  return Amount::fromString(object.at(field).asString(field), context + "." + field);
}

Amount optionalAmountField(const JsonValue &object, const std::string &field, Amount fallback, const std::string &context) {
  const JsonValue *value = object.maybe(field);
  if (value == nullptr || value->isNull()) {
    return fallback;
  }
  return Amount::fromString(value->asString(field), context + "." + field);
}

std::uint32_t optionalBps(const JsonValue &object, const std::string &field, std::uint32_t fallback) {
  const std::uint64_t value = jsonOptionalUint64(object, field, fallback);
  if (value > 10000U) {
    throw ValidationError(field + " bps out of range");
  }
  return static_cast<std::uint32_t>(value);
}

std::uint8_t requiredDecimals(const JsonValue &object) {
  const std::uint64_t value = jsonRequiredUint64(object, "decimals");
  if (value > 18U) {
    throw ValidationError("asset decimals above 18");
  }
  return static_cast<std::uint8_t>(value);
}

} // namespace

Scenario loadScenario(const std::string &path) {
  const JsonValue root = parseJsonFile(path);
  root.asObject("root");

  Scenario scenario;
  scenario.name = jsonOptionalString(root, "scenario", "unnamed");
  scenario.clock = parseClock(root);

  const auto &assetValues = root.at("assets").asArray("assets");
  for (const JsonValue &assetValue : assetValues) {
    scenario.assets.add(parseAsset(assetValue));
  }

  scenario.fees = parseFees(root);

  const auto &priceValues = root.at("prices").asArray("prices");
  for (const JsonValue &priceValue : priceValues) {
    scenario.oracles.seed(parsePricePoint(priceValue), scenario.assets);
  }

  const JsonValue *updateValues = root.maybe("priceUpdates");
  if (updateValues != nullptr && !updateValues->isNull()) {
    for (const JsonValue &updateValue : updateValues->asArray("priceUpdates")) {
      scenario.priceUpdates.push_back(parsePriceUpdate(updateValue));
    }
  }

  const auto &routeValues = root.at("routes").asArray("routes");
  for (const JsonValue &routeValue : routeValues) {
    scenario.routes.add(parseRoute(routeValue), scenario.assets);
  }

  scenario.ledger = parseLedger(root);

  const auto &orderValues = root.at("orders").asArray("orders");
  for (const JsonValue &orderValue : orderValues) {
    scenario.orders.push_back(parseOrder(orderValue));
  }

  validateScenario(scenario);
  return scenario;
}

void validateScenario(const Scenario &scenario) {
  require(!scenario.name.empty(), "scenario name is required");
  require(!scenario.assets.all().empty(), "scenario must define assets");
  require(!scenario.routes.all().empty(), "scenario must define routes");
  require(!scenario.orders.empty(), "scenario must define orders");
  scenario.ledger.validateAssets(scenario.assets);
  for (const auto &assetEntry : scenario.assets.all()) {
    require(scenario.oracles.hasPrice(assetEntry.first), "missing oracle price for asset: " + assetEntry.first);
  }
  for (const QuoteRequest &order : scenario.orders) {
    require(scenario.routes.has(order.routeId), "order references unknown route: " + order.id);
    const Route &route = scenario.routes.get(order.routeId);
    require(!route.hops.empty(), "order route has no hops: " + order.id);
    require(scenario.ledger.hasAccount(order.account), "order account is not present in ledger: " + order.account);
  }
}

Asset parseAsset(const JsonValue &value) {
  value.asObject("asset");
  Asset asset;
  asset.symbol = jsonRequiredString(value, "symbol");
  asset.displayName = jsonOptionalString(value, "name", asset.symbol);
  asset.decimals = requiredDecimals(value);
  asset.status = parseAssetStatus(jsonOptionalString(value, "status", "active"));
  asset.maxDeviationBps = optionalBps(value, "maxDeviationBps", 0U);
  asset.defaultToleranceBps = optionalBps(value, "toleranceBps", 0U);
  asset.staleAfterSeconds = jsonOptionalUint64(value, "staleAfterSeconds", 0U);
  asset.minTrade = optionalAmountField(value, "minTrade", Amount::zero(), asset.symbol);
  asset.maxTrade = optionalAmountField(value, "maxTrade", Amount::zero(), asset.symbol);
  asset.settlementEnabled = jsonOptionalBool(value, "settlementEnabled", true);
  return asset;
}

PricePoint parsePricePoint(const JsonValue &value) {
  value.asObject("price");
  PricePoint point;
  point.asset = jsonRequiredString(value, "asset");
  point.price = OracleBook::parsePrice(jsonRequiredString(value, "price"), "price." + point.asset);
  point.confidenceBps = optionalBps(value, "confidenceBps", 0U);
  point.updatedAt = jsonOptionalUint64(value, "updatedAt", 0U);
  point.sequence = jsonOptionalUint64(value, "sequence", 1U);
  point.source = jsonOptionalString(value, "source", "internal");
  return point;
}

PriceUpdate parsePriceUpdate(const JsonValue &value) {
  value.asObject("priceUpdate");
  PriceUpdate update;
  update.asset = jsonRequiredString(value, "asset");
  update.price = OracleBook::parsePrice(jsonRequiredString(value, "price"), "priceUpdate." + update.asset);
  update.confidenceBps = optionalBps(value, "confidenceBps", 0U);
  update.updatedAt = jsonOptionalUint64(value, "updatedAt", 0U);
  update.sequence = jsonRequiredUint64(value, "sequence");
  update.source = jsonOptionalString(value, "source", "internal");
  return update;
}

Route parseRoute(const JsonValue &value) {
  value.asObject("route");
  Route route;
  route.id = jsonRequiredString(value, "id");
  route.hops = jsonStringArray(value, "hops");
  route.toleranceBps = optionalBps(value, "toleranceBps", 0U);
  route.feeBps = optionalBps(value, "feeBps", 0U);
  route.settlementWindow = jsonOptionalUint64(value, "settlementWindow", 0U);
  route.enabled = jsonOptionalBool(value, "enabled", true);
  return route;
}

QuoteRequest parseOrder(const JsonValue &value) {
  value.asObject("order");
  QuoteRequest order;
  order.id = jsonRequiredString(value, "id");
  order.account = jsonRequiredString(value, "account");
  order.routeId = jsonRequiredString(value, "route");
  order.amountIn = amountField(value, "amountIn", order.id);
  order.minOut = optionalAmountField(value, "minOut", Amount::zero(), order.id);
  order.clientToleranceBps = optionalBps(value, "toleranceBps", 0U);
  order.window = jsonOptionalUint64(value, "window", 0U);
  order.settle = jsonOptionalBool(value, "settle", true);
  order.memo = jsonOptionalString(value, "memo", "");
  return order;
}

FeeSchedule parseFees(const JsonValue &root) {
  FeeSchedule fees;
  const JsonValue *value = root.maybe("fees");
  if (value == nullptr || value->isNull()) {
    return fees;
  }
  value->asObject("fees");
  fees.setDefaultBps(optionalBps(*value, "defaultBps", 0U));
  fees.setRouteBps(optionalBps(*value, "routeBps", 0U));
  fees.setWindowBps(optionalBps(*value, "windowBps", 0U));
  fees.setFeeAccount(jsonOptionalString(*value, "feeAccount", "treasury"));
  const JsonValue *overrides = value->maybe("assetOverrides");
  if (overrides != nullptr && !overrides->isNull()) {
    for (const JsonValue &entry : overrides->asArray("assetOverrides")) {
      const std::string asset = jsonRequiredString(entry, "asset");
      fees.addAssetOverride(asset, optionalBps(entry, "bps", 0U));
    }
  }
  return fees;
}

Ledger parseLedger(const JsonValue &root) {
  Ledger ledger;
  const auto &accounts = root.at("accounts").asArray("accounts");
  for (const JsonValue &accountValue : accounts) {
    accountValue.asObject("account");
    const std::string id = jsonRequiredString(accountValue, "id");
    ledger.ensureAccount(id);
    const JsonValue *balances = accountValue.maybe("balances");
    if (balances == nullptr || balances->isNull()) {
      continue;
    }
    for (const JsonValue &balanceValue : balances->asArray("balances")) {
      balanceValue.asObject("balance");
      const std::string asset = jsonRequiredString(balanceValue, "asset");
      const Amount amount = amountField(balanceValue, "amount", id + "." + asset);
      ledger.credit(id, asset, amount, "seed");
    }
  }
  return ledger;
}

Clock parseClock(const JsonValue &root) {
  Clock clock;
  const JsonValue *value = root.maybe("clock");
  if (value == nullptr || value->isNull()) {
    return clock;
  }
  value->asObject("clock");
  clock.timestamp = jsonOptionalUint64(*value, "timestamp", 0U);
  clock.window = jsonOptionalUint64(*value, "window", 0U);
  return clock;
}

} // namespace aurora
