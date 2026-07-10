#pragma once

#include "asset.hpp"
#include "fees.hpp"
#include "oracle.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct Route {
  std::string id;
  std::vector<std::string> hops;
  std::uint32_t toleranceBps{0};
  std::uint32_t feeBps{0};
  std::uint64_t settlementWindow{0};
  bool enabled{true};
};

struct RouteLegQuote {
  std::string source;
  std::string destination;
  Amount input{Amount::zero()};
  Amount normalized18{Amount::zero()};
  Amount output{Amount::zero()};
  long double sourcePrice{0.0L};
  long double destinationPrice{0.0L};
};

class RouteBook {
public:
  void add(Route route, const AssetRegistry &assets);
  bool has(const std::string &id) const;
  const Route &get(const std::string &id) const;
  const std::map<std::string, Route> &all() const;
  std::vector<RouteLegQuote> simulate(const Route &route, Amount amount, const AssetRegistry &assets, const OracleBook &oracles, const Clock &clock) const;
  void validateRoute(const Route &route, const AssetRegistry &assets) const;

private:
  std::map<std::string, Route> routes_;
};

std::string routePair(const Route &route);

} // namespace aurora
