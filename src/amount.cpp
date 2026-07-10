#include "amount.hpp"

#include <array>

namespace aurora {

Amount::Amount() = default;

Amount::Amount(std::uint64_t raw) : raw_(raw) {}

Amount Amount::zero() {
  return Amount(0);
}

Amount Amount::fromString(const std::string &value, const std::string &field) {
  return Amount(parseUint64(value, field));
}

Amount Amount::fromUnits(long double units, std::uint8_t decimals) {
  return convertWholeToRaw(units, decimals, "amount");
}

Amount Amount::fromRounded(long double raw, const std::string &field) {
  if (!std::isfinite(static_cast<double>(raw)) || raw < 0.0L) {
    throw ValidationError("invalid numeric amount for " + field);
  }
  const long double rounded = std::floor(raw + 0.5L);
  const long double maxValue = static_cast<long double>(std::numeric_limits<std::uint64_t>::max());
  if (rounded > maxValue) {
    throw ValidationError("amount overflow for " + field);
  }
  return Amount(static_cast<std::uint64_t>(rounded));
}

std::uint64_t Amount::raw() const {
  return raw_;
}

std::string Amount::str() const {
  return std::to_string(raw_);
}

bool Amount::isZero() const {
  return raw_ == 0;
}

Amount Amount::checkedAdd(Amount other, const std::string &field) const {
  if (raw_ > std::numeric_limits<std::uint64_t>::max() - other.raw_) {
    throw ExecutionError("amount overflow while adding " + field);
  }
  return Amount(raw_ + other.raw_);
}

Amount Amount::checkedSub(Amount other, const std::string &field) const {
  if (raw_ < other.raw_) {
    throw ExecutionError("amount underflow while subtracting " + field);
  }
  return Amount(raw_ - other.raw_);
}

Amount Amount::min(Amount other) const {
  return raw_ < other.raw_ ? *this : other;
}

Amount Amount::max(Amount other) const {
  return raw_ > other.raw_ ? *this : other;
}

Amount Amount::applyBps(std::uint32_t bps) const {
  const long double value = static_cast<long double>(raw_) * static_cast<long double>(bps) / 10000.0L;
  return Amount::fromRounded(value, "basis-points");
}

Amount Amount::reduceBps(std::uint32_t bps) const {
  if (bps >= 10000U) {
    return Amount::zero();
  }
  return checkedSub(applyBps(bps), "reduced basis-points");
}

Amount Amount::increaseBps(std::uint32_t bps) const {
  return checkedAdd(applyBps(bps), "increased basis-points");
}

bool Amount::operator==(Amount other) const {
  return raw_ == other.raw_;
}

bool Amount::operator!=(Amount other) const {
  return raw_ != other.raw_;
}

bool Amount::operator<(Amount other) const {
  return raw_ < other.raw_;
}

bool Amount::operator<=(Amount other) const {
  return raw_ <= other.raw_;
}

bool Amount::operator>(Amount other) const {
  return raw_ > other.raw_;
}

bool Amount::operator>=(Amount other) const {
  return raw_ >= other.raw_;
}

DecimalScale scaleFor(std::uint8_t decimals) {
  return DecimalScale{decimals, pow10u(decimals)};
}

std::uint64_t pow10u(std::uint8_t decimals) {
  static const std::array<std::uint64_t, 19> factors{
      1ULL,
      10ULL,
      100ULL,
      1000ULL,
      10000ULL,
      100000ULL,
      1000000ULL,
      10000000ULL,
      100000000ULL,
      1000000000ULL,
      10000000000ULL,
      100000000000ULL,
      1000000000000ULL,
      10000000000000ULL,
      100000000000000ULL,
      1000000000000000ULL,
      10000000000000000ULL,
      100000000000000000ULL,
      1000000000000000000ULL,
  };
  if (decimals >= factors.size()) {
    throw ValidationError("decimal precision above 18 is not supported");
  }
  return factors[decimals];
}

long double pow10ld(std::uint8_t decimals) {
  return static_cast<long double>(pow10u(decimals));
}

long double toWholeUnits(Amount amount, std::uint8_t decimals) {
  return static_cast<long double>(amount.raw()) / pow10ld(decimals);
}

std::string formatUnits(Amount amount, std::uint8_t decimals) {
  const std::uint64_t factor = pow10u(decimals);
  const std::uint64_t whole = amount.raw() / factor;
  std::uint64_t fraction = amount.raw() % factor;
  if (decimals == 0U) {
    return std::to_string(whole);
  }
  std::string fractionText = std::to_string(fraction);
  if (fractionText.size() < decimals) {
    fractionText = repeat('0', static_cast<std::size_t>(decimals) - fractionText.size()) + fractionText;
  }
  while (!fractionText.empty() && fractionText.back() == '0') {
    fractionText.pop_back();
  }
  if (fractionText.empty()) {
    return std::to_string(whole);
  }
  return std::to_string(whole) + "." + fractionText;
}

Amount convertWholeToRaw(long double units, std::uint8_t decimals, const std::string &field) {
  const long double raw = units * pow10ld(decimals);
  return Amount::fromRounded(raw, field);
}

Amount normalized18FromWhole(long double units, const std::string &field) {
  return convertWholeToRaw(units, 18U, field);
}

Amount rawFromNormalized18(Amount normalized, std::uint8_t decimals, const std::string &field) {
  const long double units = toWholeUnits(normalized, 18U);
  return convertWholeToRaw(units, decimals, field);
}

std::uint32_t bpsDiff(Amount left, Amount right) {
  const Amount high = left.max(right);
  const Amount low = left.min(right);
  if (high.isZero()) {
    return 0U;
  }
  const Amount delta = high.checkedSub(low, "basis-point delta");
  const long double ratio = static_cast<long double>(delta.raw()) * 10000.0L / static_cast<long double>(high.raw());
  return static_cast<std::uint32_t>(std::floor(ratio + 0.5L));
}

std::uint32_t bpsRatio(Amount numerator, Amount denominator) {
  if (denominator.isZero()) {
    return 0U;
  }
  const long double ratio = static_cast<long double>(numerator.raw()) * 10000.0L / static_cast<long double>(denominator.raw());
  return static_cast<std::uint32_t>(std::floor(ratio + 0.5L));
}

ConversionResult convertByPrice(
    Amount sourceRaw,
    std::uint8_t sourceDecimals,
    std::uint8_t destinationDecimals,
    long double sourcePrice,
    long double destinationPrice,
    const std::string &field) {
  if (sourcePrice <= 0.0L || destinationPrice <= 0.0L) {
    throw ValidationError("price must be positive for " + field);
  }
  const long double sourceUnits = toWholeUnits(sourceRaw, sourceDecimals);
  const long double destinationUnits = sourceUnits * sourcePrice / destinationPrice;
  ConversionResult result;
  result.destinationUnits = destinationUnits;
  result.normalized18 = normalized18FromWhole(destinationUnits, field + ".normalized");
  result.destinationRaw = convertWholeToRaw(destinationUnits, destinationDecimals, field + ".raw");
  return result;
}

} // namespace aurora
