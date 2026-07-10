#pragma once

#include "amount.hpp"
#include "common.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct WindowPolicy {
  std::uint64_t firstWindow{0};
  std::uint64_t lastWindow{0};
  std::uint64_t cadenceSeconds{60};
  std::uint32_t carryBps{0};
  std::uint32_t congestionBps{0};
  Amount softCapacity{Amount::zero()};
  Amount hardCapacity{Amount::zero()};
};

struct WindowBucket {
  std::uint64_t window{0};
  Amount quoted{Amount::zero()};
  Amount settled{Amount::zero()};
  Amount rejected{Amount::zero()};
  std::uint32_t congestionBps{0};
};

struct WindowAdmission {
  bool accepted{false};
  std::uint64_t window{0};
  std::string reason;
  Amount projected{Amount::zero()};
  std::uint32_t surchargeBps{0};
};

class WindowBook {
public:
  explicit WindowBook(WindowPolicy policy);

  const WindowPolicy &policy() const;
  WindowAdmission admit(std::uint64_t requestedWindow, Amount amount) const;
  void recordQuote(std::uint64_t window, Amount amount);
  void recordSettlement(std::uint64_t window, Amount amount);
  void recordRejection(std::uint64_t window, Amount amount);
  WindowBucket bucket(std::uint64_t window) const;
  std::vector<WindowBucket> buckets() const;
  std::uint64_t normalizeWindow(std::uint64_t requestedWindow) const;
  std::uint32_t surchargeFor(std::uint64_t window, Amount projected) const;

private:
  WindowBucket &bucketMutable(std::uint64_t window);

  WindowPolicy policy_;
  std::map<std::uint64_t, WindowBucket> buckets_;
};

std::uint64_t windowFromTimestamp(std::uint64_t timestamp, std::uint64_t cadenceSeconds);
std::string windowLabel(std::uint64_t window);
WindowPolicy defaultWindowPolicy(std::uint64_t currentWindow);

} // namespace aurora
