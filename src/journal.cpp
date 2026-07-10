#include "journal.hpp"

namespace aurora {

Journal::Journal(std::string namespaceId) : ids_(std::move(namespaceId)) {}

JournalRecord Journal::append(
    std::uint64_t window,
    const std::string &type,
    const std::string &subject,
    const std::string &asset,
    Amount amount,
    const std::string &ref) {
  require(!type.empty(), "journal record type is required");
  require(!subject.empty(), "journal record subject is required");
  JournalRecord record;
  record.sequence = static_cast<std::uint64_t>(records_.size()) + 1U;
  record.window = window;
  record.type = type;
  record.subject = subject;
  record.asset = asset;
  record.amount = amount;
  record.ref = ref;
  record.previousHash = records_.empty() ? "genesis" : records_.back().hash;
  record.id = ids_.scoped(type, subject, record.sequence);
  record.hash = computeHash(record);
  records_.push_back(record);
  return record;
}

void Journal::appendLedgerEvents(std::uint64_t window, const std::vector<LedgerEvent> &events) {
  for (const LedgerEvent &event : events) {
    append(window, "ledger." + event.type, event.account, event.asset, event.amount, event.ref);
  }
}

void Journal::appendSettlementEvents(const std::vector<SettlementEvent> &events) {
  for (const SettlementEvent &event : events) {
    append(0U, "settlement." + event.type, event.account.empty() ? event.id : event.account, event.asset, event.amount, event.id);
  }
}

JournalPage Journal::page(const JournalCursor &cursor) const {
  JournalPage page;
  const std::uint64_t limit = cursor.limit == 0U ? 100U : cursor.limit;
  for (const JournalRecord &record : records_) {
    if (record.sequence <= cursor.afterSequence) {
      continue;
    }
    if (!cursor.type.empty() && record.type != cursor.type) {
      continue;
    }
    if (!cursor.subject.empty() && record.subject != cursor.subject) {
      continue;
    }
    if (page.records.size() >= limit) {
      page.hasMore = true;
      break;
    }
    page.nextSequence = record.sequence;
    page.records.push_back(record);
  }
  return page;
}

std::vector<JournalRecord> Journal::byType(const std::string &type) const {
  std::vector<JournalRecord> result;
  for (const JournalRecord &record : records_) {
    if (record.type == type) {
      result.push_back(record);
    }
  }
  return result;
}

std::vector<JournalRecord> Journal::bySubject(const std::string &subject) const {
  std::vector<JournalRecord> result;
  for (const JournalRecord &record : records_) {
    if (record.subject == subject) {
      result.push_back(record);
    }
  }
  return result;
}

const std::vector<JournalRecord> &Journal::records() const {
  return records_;
}

bool Journal::verifyHashChain() const {
  std::string previous = "genesis";
  for (const JournalRecord &record : records_) {
    if (record.previousHash != previous) {
      return false;
    }
    if (record.hash != computeHash(record)) {
      return false;
    }
    previous = record.hash;
  }
  return true;
}

std::string Journal::tipHash() const {
  if (records_.empty()) {
    return "genesis";
  }
  return records_.back().hash;
}

std::string Journal::recordPayload(const JournalRecord &record) const {
  return record.id.str() + "|" +
         std::to_string(record.sequence) + "|" +
         std::to_string(record.window) + "|" +
         record.type + "|" +
         record.subject + "|" +
         record.asset + "|" +
         record.amount.str() + "|" +
         record.ref + "|" +
         record.previousHash;
}

std::string Journal::computeHash(const JournalRecord &record) const {
  return hex64(fnv1a64(recordPayload(record)));
}

void JournalIndex::rebuild(const std::vector<JournalRecord> &records) {
  byType_.clear();
  bySubject_.clear();
  byAsset_.clear();
  for (const JournalRecord &record : records) {
    byType_[record.type].push_back(record.sequence);
    bySubject_[record.subject].push_back(record.sequence);
    if (!record.asset.empty()) {
      byAsset_[record.asset].push_back(record.sequence);
    }
  }
}

std::vector<std::uint64_t> JournalIndex::findByType(const std::string &type) const {
  auto it = byType_.find(type);
  if (it == byType_.end()) {
    return {};
  }
  return it->second;
}

std::vector<std::uint64_t> JournalIndex::findBySubject(const std::string &subject) const {
  auto it = bySubject_.find(subject);
  if (it == bySubject_.end()) {
    return {};
  }
  return it->second;
}

std::vector<std::uint64_t> JournalIndex::findByAsset(const std::string &asset) const {
  auto it = byAsset_.find(asset);
  if (it == byAsset_.end()) {
    return {};
  }
  return it->second;
}

bool JournalIndex::empty() const {
  return byType_.empty() && bySubject_.empty() && byAsset_.empty();
}

std::size_t JournalIndex::size() const {
  return byType_.size() + bySubject_.size() + byAsset_.size();
}

} // namespace aurora
