#pragma once

#include "ledger.hpp"
#include "settlement.hpp"

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace aurora {

struct CheckpointPayload {
  std::uint64_t sequence{0};
  std::uint64_t window{0};
  std::string previousDigest;
  std::string scenario;
  std::vector<BalanceRow> balances;
  std::vector<SettlementEvent> events;
};

struct IntegrityCheckpoint {
  std::uint64_t sequence{0};
  std::uint64_t window{0};
  std::string previousDigest;
  std::string digest;
  std::string scenario;
  std::uint64_t balanceRows{0};
  std::uint64_t eventCount{0};
  std::uint64_t totalRaw{0};
};

class CheckpointBuilder {
public:
  static IntegrityCheckpoint build(CheckpointPayload payload);
  static std::string canonicalize(CheckpointPayload payload);
  static std::string digest256(const std::string &canonical);
};

struct PendingCheckpoint {
  IntegrityCheckpoint checkpoint;
  std::string proposer;
  std::set<std::string> approvals;
};

class CheckpointRegistry {
public:
  explicit CheckpointRegistry(std::uint32_t quorum);

  void addReviewer(const std::string &reviewer);
  void removeReviewer(const std::string &reviewer);
  void setQuorum(std::uint32_t quorum);
  IntegrityCheckpoint propose(const std::string &reviewer, CheckpointPayload payload);
  bool approve(const std::string &reviewer, const std::string &digest);
  void cancel(const std::string &reviewer, const std::string &digest);

  std::uint32_t quorum() const;
  bool isReviewer(const std::string &reviewer) const;
  bool hasPending() const;
  const PendingCheckpoint &pending() const;
  const std::vector<IntegrityCheckpoint> &finalized() const;
  std::string latestDigest() const;

private:
  void validatePayload(const CheckpointPayload &payload) const;
  bool finalizeIfReady();

  std::uint32_t quorum_{0};
  std::set<std::string> reviewers_;
  std::optional<PendingCheckpoint> pending_;
  std::vector<IntegrityCheckpoint> finalized_;
};

} // namespace aurora
