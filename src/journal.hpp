#pragma once

#include "ids.hpp"
#include "ledger.hpp"
#include "settlement.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct JournalRecord {
  StableId id;
  std::uint64_t sequence{0};
  std::uint64_t window{0};
  std::string type;
  std::string subject;
  std::string asset;
  Amount amount{Amount::zero()};
  std::string ref;
  std::string previousHash;
  std::string hash;
};

struct JournalCursor {
  std::uint64_t afterSequence{0};
  std::uint64_t limit{100};
  std::string type;
  std::string subject;
};

struct JournalPage {
  std::vector<JournalRecord> records;
  std::uint64_t nextSequence{0};
  bool hasMore{false};
};

class Journal {
public:
  explicit Journal(std::string namespaceId);

  JournalRecord append(
      std::uint64_t window,
      const std::string &type,
      const std::string &subject,
      const std::string &asset,
      Amount amount,
      const std::string &ref);

  void appendLedgerEvents(std::uint64_t window, const std::vector<LedgerEvent> &events);
  void appendSettlementEvents(const std::vector<SettlementEvent> &events);
  JournalPage page(const JournalCursor &cursor) const;
  std::vector<JournalRecord> byType(const std::string &type) const;
  std::vector<JournalRecord> bySubject(const std::string &subject) const;
  const std::vector<JournalRecord> &records() const;
  bool verifyHashChain() const;
  std::string tipHash() const;

private:
  std::string recordPayload(const JournalRecord &record) const;
  std::string computeHash(const JournalRecord &record) const;

  IdGenerator ids_;
  std::vector<JournalRecord> records_;
};

class JournalIndex {
public:
  void rebuild(const std::vector<JournalRecord> &records);
  std::vector<std::uint64_t> findByType(const std::string &type) const;
  std::vector<std::uint64_t> findBySubject(const std::string &subject) const;
  std::vector<std::uint64_t> findByAsset(const std::string &asset) const;
  bool empty() const;
  std::size_t size() const;

private:
  std::map<std::string, std::vector<std::uint64_t>> byType_;
  std::map<std::string, std::vector<std::uint64_t>> bySubject_;
  std::map<std::string, std::vector<std::uint64_t>> byAsset_;
};

} // namespace aurora
