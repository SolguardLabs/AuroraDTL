#include "window.hpp"

namespace aurora {

WindowBook::WindowBook(WindowPolicy policy) : policy_(policy) {
  if (policy_.cadenceSeconds == 0U) {
    policy_.cadenceSeconds = 60U;
  }
  require(policy_.carryBps <= 10000U, "window carry bps out of range");
  require(policy_.congestionBps <= 10000U, "window congestion bps out of range");
  if (policy_.lastWindow != 0U) {
    require(policy_.lastWindow >= policy_.firstWindow, "window policy has inverted range");
  }
  if (!policy_.hardCapacity.isZero()) {
    require(policy_.hardCapacity >= policy_.softCapacity, "window hard capacity below soft capacity");
  }
}

const WindowPolicy &WindowBook::policy() const {
  return policy_;
}

WindowAdmission WindowBook::admit(std::uint64_t requestedWindow, Amount amount) const {
  WindowAdmission admission;
  admission.window = normalizeWindow(requestedWindow);
  const WindowBucket current = bucket(admission.window);
  admission.projected = current.quoted.checkedAdd(amount, "window projected quote");
  admission.surchargeBps = surchargeFor(admission.window, admission.projected);
  if (policy_.lastWindow != 0U && admission.window > policy_.lastWindow) {
    admission.accepted = false;
    admission.reason = "window-closed";
    return admission;
  }
  if (!policy_.hardCapacity.isZero() && admission.projected > policy_.hardCapacity) {
    admission.accepted = false;
    admission.reason = "window-capacity";
    return admission;
  }
  admission.accepted = true;
  admission.reason = "accepted";
  return admission;
}

void WindowBook::recordQuote(std::uint64_t window, Amount amount) {
  WindowBucket &target = bucketMutable(normalizeWindow(window));
  target.quoted = target.quoted.checkedAdd(amount, "window quote");
  target.congestionBps = surchargeFor(target.window, target.quoted);
}

void WindowBook::recordSettlement(std::uint64_t window, Amount amount) {
  WindowBucket &target = bucketMutable(normalizeWindow(window));
  target.settled = target.settled.checkedAdd(amount, "window settlement");
  target.congestionBps = surchargeFor(target.window, target.quoted);
}

void WindowBook::recordRejection(std::uint64_t window, Amount amount) {
  WindowBucket &target = bucketMutable(normalizeWindow(window));
  target.rejected = target.rejected.checkedAdd(amount, "window rejection");
  target.congestionBps = surchargeFor(target.window, target.quoted);
}

WindowBucket WindowBook::bucket(std::uint64_t window) const {
  const std::uint64_t normalized = normalizeWindow(window);
  auto it = buckets_.find(normalized);
  if (it == buckets_.end()) {
    WindowBucket empty;
    empty.window = normalized;
    return empty;
  }
  return it->second;
}

std::vector<WindowBucket> WindowBook::buckets() const {
  std::vector<WindowBucket> result;
  result.reserve(buckets_.size());
  for (const auto &entry : buckets_) {
    result.push_back(entry.second);
  }
  return result;
}

std::uint64_t WindowBook::normalizeWindow(std::uint64_t requestedWindow) const {
  if (requestedWindow == 0U) {
    return policy_.firstWindow;
  }
  if (requestedWindow < policy_.firstWindow) {
    return policy_.firstWindow;
  }
  return requestedWindow;
}

std::uint32_t WindowBook::surchargeFor(std::uint64_t window, Amount projected) const {
  (void)window;
  if (policy_.softCapacity.isZero() || projected <= policy_.softCapacity) {
    return 0U;
  }
  if (policy_.hardCapacity.isZero() || projected >= policy_.hardCapacity) {
    return policy_.congestionBps;
  }
  const Amount overSoft = projected.checkedSub(policy_.softCapacity, "window soft overage");
  const Amount range = policy_.hardCapacity.checkedSub(policy_.softCapacity, "window capacity range");
  const std::uint32_t ratio = bpsRatio(overSoft, range);
  const std::uint64_t surcharge =
      static_cast<std::uint64_t>(policy_.congestionBps) * static_cast<std::uint64_t>(ratio) / 10000U;
  return surcharge > 10000U ? 10000U : static_cast<std::uint32_t>(surcharge);
}

WindowBucket &WindowBook::bucketMutable(std::uint64_t window) {
  const std::uint64_t normalized = normalizeWindow(window);
  auto inserted = buckets_.try_emplace(normalized);
  WindowBucket &bucket = inserted.first->second;
  bucket.window = normalized;
  return bucket;
}

std::uint64_t windowFromTimestamp(std::uint64_t timestamp, std::uint64_t cadenceSeconds) {
  if (cadenceSeconds == 0U) {
    cadenceSeconds = 60U;
  }
  return timestamp / cadenceSeconds;
}

std::string windowLabel(std::uint64_t window) {
  return "window-" + std::to_string(window);
}

WindowPolicy defaultWindowPolicy(std::uint64_t currentWindow) {
  WindowPolicy policy;
  policy.firstWindow = currentWindow;
  policy.lastWindow = currentWindow + 16U;
  policy.cadenceSeconds = 60U;
  policy.carryBps = 25U;
  policy.congestionBps = 40U;
  policy.softCapacity = Amount(5000000000000000ULL);
  policy.hardCapacity = Amount(9000000000000000ULL);
  return policy;
}

} // namespace aurora
