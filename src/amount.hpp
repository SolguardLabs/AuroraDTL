#pragma once

#include "common.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

namespace aurora {

class Amount {
public:
  Amount();
  explicit Amount(std::uint64_t raw);

  static Amount zero();
  static Amount fromString(const std::string &value, const std::string &field);
  static Amount fromUnits(long double units, std::uint8_t decimals);
  static Amount fromRounded(long double raw, const std::string &field);

  std::uint64_t raw() const;
  std::string str() const;
  bool isZero() const;

  Amount checkedAdd(Amount other, const std::string &field) const;
  Amount checkedSub(Amount other, const std::string &field) const;
  Amount min(Amount other) const;
  Amount max(Amount other) const;
  Amount applyBps(std::uint32_t bps) const;
  Amount reduceBps(std::uint32_t bps) const;
  Amount increaseBps(std::uint32_t bps) const;

  bool operator==(Amount other) const;
  bool operator!=(Amount other) const;
  bool operator<(Amount other) const;
  bool operator<=(Amount other) const;
  bool operator>(Amount other) const;
  bool operator>=(Amount other) const;

private:
  std::uint64_t raw_{0};
};

struct DecimalScale {
  std::uint8_t decimals{0};
  std::uint64_t factor{1};
};

DecimalScale scaleFor(std::uint8_t decimals);
std::uint64_t pow10u(std::uint8_t decimals);
long double pow10ld(std::uint8_t decimals);
long double toWholeUnits(Amount amount, std::uint8_t decimals);
std::string formatUnits(Amount amount, std::uint8_t decimals);
Amount convertWholeToRaw(long double units, std::uint8_t decimals, const std::string &field);
Amount normalized18FromWhole(long double units, const std::string &field);
Amount rawFromNormalized18(Amount normalized, std::uint8_t decimals, const std::string &field);
std::uint32_t bpsDiff(Amount left, Amount right);
std::uint32_t bpsRatio(Amount numerator, Amount denominator);

struct ConversionResult {
  Amount normalized18;
  Amount destinationRaw;
  long double destinationUnits{0.0L};
};

ConversionResult convertByPrice(
    Amount sourceRaw,
    std::uint8_t sourceDecimals,
    std::uint8_t destinationDecimals,
    long double sourcePrice,
    long double destinationPrice,
    const std::string &field);

} // namespace aurora
