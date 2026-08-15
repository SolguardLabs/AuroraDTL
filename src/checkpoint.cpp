#include "checkpoint.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <limits>
#include <sstream>

namespace aurora {

namespace {

std::uint64_t fnv1a(const std::string &value, std::uint64_t basis) {
  std::uint64_t hash = basis;
  for (unsigned char character : value) {
    hash ^= static_cast<std::uint64_t>(character);
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string hex64(std::uint64_t value) {
  std::ostringstream out;
  out << std::hex << std::setfill('0') << std::setw(16) << value;
  return out.str();
}

bool balanceLess(const BalanceRow &left, const BalanceRow &right) {
  if (left.account != right.account) {
    return left.account < right.account;
  }
  if (left.asset != right.asset) {
    return left.asset < right.asset;
  }
  return left.amount.raw() < right.amount.raw();
}

} // namespace

IntegrityCheckpoint CheckpointBuilder::build(CheckpointPayload payload) {
  require(payload.sequence > 0U, "checkpoint sequence must be positive");
  require(payload.window > 0U, "checkpoint window must be positive");
  require(!payload.scenario.empty(), "checkpoint scenario is required");

  IntegrityCheckpoint checkpoint;
  checkpoint.sequence = payload.sequence;
  checkpoint.window = payload.window;
  checkpoint.previousDigest = payload.previousDigest;
  checkpoint.scenario = payload.scenario;
  checkpoint.balanceRows = static_cast<std::uint64_t>(payload.balances.size());
  checkpoint.eventCount = static_cast<std::uint64_t>(payload.events.size());
  for (const BalanceRow &row : payload.balances) {
    if (checkpoint.totalRaw > std::numeric_limits<std::uint64_t>::max() - row.amount.raw()) {
      checkpoint.totalRaw = std::numeric_limits<std::uint64_t>::max();
      break;
    }
    checkpoint.totalRaw += row.amount.raw();
  }
  checkpoint.digest = digest256(canonicalize(std::move(payload)));
  return checkpoint;
}

std::string CheckpointBuilder::canonicalize(CheckpointPayload payload) {
  std::sort(payload.balances.begin(), payload.balances.end(), balanceLess);
  std::ostringstream out;
  out << "aurora-checkpoint-v1\n";
  out << "sequence=" << payload.sequence << "\n";
  out << "window=" << payload.window << "\n";
  out << "previous=" << payload.previousDigest << "\n";
  out << "scenario=" << payload.scenario << "\n";
  for (const BalanceRow &row : payload.balances) {
    out << "balance=" << row.account << "|" << row.asset << "|" << row.amount.str() << "\n";
  }
  for (const SettlementEvent &event : payload.events) {
    out << "event=" << event.type << "|" << event.id << "|" << event.account << "|" << event.asset << "|"
        << event.amount.str() << "|" << event.note << "\n";
  }
  return out.str();
}

std::string CheckpointBuilder::digest256(const std::string &canonical) {
  const std::array<std::uint64_t, 4> seeds{
      1469598103934665603ULL,
      7809847782465536322ULL,
      1609587929392839161ULL,
      9650029242287828579ULL,
  };
  std::string digest;
  digest.reserve(64U);
  for (std::uint64_t seed : seeds) {
    digest += hex64(fnv1a(canonical, seed));
  }
  return digest;
}

CheckpointRegistry::CheckpointRegistry(std::uint32_t quorum) : quorum_(quorum) {
  require(quorum > 0U, "checkpoint quorum must be positive");
}

void CheckpointRegistry::addReviewer(const std::string &reviewer) {
  require(isIdentifier(reviewer), "checkpoint reviewer id is invalid: " + reviewer);
  const bool inserted = reviewers_.insert(reviewer).second;
  require(inserted, "checkpoint reviewer already exists: " + reviewer);
}

void CheckpointRegistry::removeReviewer(const std::string &reviewer) {
  require(reviewers_.find(reviewer) != reviewers_.end(), "checkpoint reviewer does not exist: " + reviewer);
  require(reviewers_.size() - 1U >= quorum_, "checkpoint reviewer removal would violate quorum");
  if (pending_.has_value()) {
    require(pending_->approvals.find(reviewer) == pending_->approvals.end(), "approved reviewer cannot be removed while checkpoint is pending");
  }
  reviewers_.erase(reviewer);
}

void CheckpointRegistry::setQuorum(std::uint32_t quorum) {
  require(quorum > 0U, "checkpoint quorum must be positive");
  require(quorum <= reviewers_.size(), "checkpoint quorum exceeds reviewer count");
  quorum_ = quorum;
  finalizeIfReady();
}

IntegrityCheckpoint CheckpointRegistry::propose(const std::string &reviewer, CheckpointPayload payload) {
  require(isReviewer(reviewer), "checkpoint proposer is not a reviewer: " + reviewer);
  require(!pending_.has_value(), "checkpoint proposal already pending");
  validatePayload(payload);
  PendingCheckpoint proposal;
  proposal.checkpoint = CheckpointBuilder::build(std::move(payload));
  proposal.proposer = reviewer;
  proposal.approvals.insert(reviewer);
  const IntegrityCheckpoint result = proposal.checkpoint;
  pending_ = std::move(proposal);
  finalizeIfReady();
  return result;
}

bool CheckpointRegistry::approve(const std::string &reviewer, const std::string &digest) {
  require(isReviewer(reviewer), "checkpoint approver is not a reviewer: " + reviewer);
  require(pending_.has_value(), "no checkpoint proposal pending");
  require(pending_->checkpoint.digest == digest, "checkpoint digest does not match pending proposal");
  const bool inserted = pending_->approvals.insert(reviewer).second;
  require(inserted, "checkpoint reviewer already approved: " + reviewer);
  return finalizeIfReady();
}

void CheckpointRegistry::cancel(const std::string &reviewer, const std::string &digest) {
  require(pending_.has_value(), "no checkpoint proposal pending");
  require(pending_->proposer == reviewer, "only checkpoint proposer can cancel");
  require(pending_->checkpoint.digest == digest, "checkpoint digest does not match pending proposal");
  pending_.reset();
}

std::uint32_t CheckpointRegistry::quorum() const {
  return quorum_;
}

bool CheckpointRegistry::isReviewer(const std::string &reviewer) const {
  return reviewers_.find(reviewer) != reviewers_.end();
}

bool CheckpointRegistry::hasPending() const {
  return pending_.has_value();
}

const PendingCheckpoint &CheckpointRegistry::pending() const {
  require(pending_.has_value(), "no checkpoint proposal pending");
  return *pending_;
}

const std::vector<IntegrityCheckpoint> &CheckpointRegistry::finalized() const {
  return finalized_;
}

std::string CheckpointRegistry::latestDigest() const {
  return finalized_.empty() ? "" : finalized_.back().digest;
}

void CheckpointRegistry::validatePayload(const CheckpointPayload &payload) const {
  const std::uint64_t expectedSequence = finalized_.empty() ? 1U : finalized_.back().sequence + 1U;
  require(payload.sequence == expectedSequence, "checkpoint sequence is not contiguous");
  require(payload.previousDigest == latestDigest(), "checkpoint previous digest does not match finalized chain");
}

bool CheckpointRegistry::finalizeIfReady() {
  if (!pending_.has_value() || pending_->approvals.size() < quorum_) {
    return false;
  }
  finalized_.push_back(pending_->checkpoint);
  pending_.reset();
  return true;
}

} // namespace aurora
