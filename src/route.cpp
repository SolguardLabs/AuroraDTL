#include "route.hpp"

namespace aurora {

void RouteBook::add(Route route, const AssetRegistry &assets) {
  validateRoute(route, assets);
  const auto inserted = routes_.emplace(route.id, std::move(route));
  if (!inserted.second) {
    throw ValidationError("duplicate route: " + inserted.first->first);
  }
}

bool RouteBook::has(const std::string &id) const {
  return routes_.find(id) != routes_.end();
}

const Route &RouteBook::get(const std::string &id) const {
  return lookup(routes_, id, "route");
}

const std::map<std::string, Route> &RouteBook::all() const {
  return routes_;
}

std::vector<RouteLegQuote> RouteBook::simulate(
    const Route &route,
    Amount amount,
    const AssetRegistry &assets,
    const OracleBook &oracles,
    const Clock &clock) const {
  if (!route.enabled) {
    throw ValidationError("route is disabled: " + route.id);
  }
  require(route.hops.size() >= 2U, "route must contain at least two hops: " + route.id);
  std::vector<RouteLegQuote> legs;
  legs.reserve(route.hops.size() - 1U);
  Amount cursor = amount;
  for (std::size_t i = 0; i + 1U < route.hops.size(); ++i) {
    const Asset &source = assets.get(route.hops[i]);
    const Asset &destination = assets.get(route.hops[i + 1U]);
    assets.validateTradable(source.symbol, cursor);
    assets.validateSettlement(destination.symbol);
    oracles.validateFresh(source.symbol, assets, clock);
    oracles.validateFresh(destination.symbol, assets, clock);
    const long double sourcePrice = oracles.numericPrice(source.symbol);
    const long double destinationPrice = oracles.numericPrice(destination.symbol);
    const ConversionResult conversion = convertByPrice(
        cursor,
        source.decimals,
        destination.decimals,
        sourcePrice,
        destinationPrice,
        route.id + ".leg" + std::to_string(i));
    RouteLegQuote leg;
    leg.source = source.symbol;
    leg.destination = destination.symbol;
    leg.input = cursor;
    leg.normalized18 = conversion.normalized18;
    leg.output = conversion.destinationRaw;
    leg.sourcePrice = sourcePrice;
    leg.destinationPrice = destinationPrice;
    legs.push_back(leg);
    cursor = conversion.destinationRaw;
  }
  return legs;
}

void RouteBook::validateRoute(const Route &route, const AssetRegistry &assets) const {
  require(isIdentifier(route.id), "route id is not a valid identifier: " + route.id);
  require(route.hops.size() >= 2U, "route must contain at least two hops: " + route.id);
  require(route.hops.size() <= 8U, "route contains too many hops: " + route.id);
  require(route.toleranceBps <= 10000U, "route tolerance bps out of range: " + route.id);
  require(route.feeBps <= 10000U, "route fee bps out of range: " + route.id);
  for (const std::string &symbol : route.hops) {
    require(assets.has(symbol), "route references unknown asset " + symbol + ": " + route.id);
  }
  for (std::size_t i = 1; i < route.hops.size(); ++i) {
    require(route.hops[i] != route.hops[i - 1U], "route contains adjacent duplicate asset: " + route.id);
  }
}

std::string routePair(const Route &route) {
  if (route.hops.empty()) {
    return "";
  }
  return route.hops.front() + "/" + route.hops.back();
}

} // namespace aurora
