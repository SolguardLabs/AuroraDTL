#pragma once

#include "risk.hpp"
#include "window.hpp"

#include <string>
#include <vector>

namespace aurora {

struct AuditItem {
  std::string section;
  std::string key;
  std::string value;
};

struct AuditSummary {
  std::vector<AuditItem> items;
  std::vector<RiskFinding> findings;

  void add(std::string section, std::string key, std::string value);
  std::string valueOf(const std::string &section, const std::string &key) const;
};

class CatalogAuditor {
public:
  CatalogAuditor(const AssetRegistry &assets, const OracleBook &oracles, const RouteBook &routes, const FeeSchedule &fees);

  AuditSummary summarize() const;
  void summarizeAssets(AuditSummary &summary) const;
  void summarizeRoutes(AuditSummary &summary) const;
  void summarizePrices(AuditSummary &summary) const;
  void summarizeFees(AuditSummary &summary) const;

private:
  const AssetRegistry &assets_;
  const OracleBook &oracles_;
  const RouteBook &routes_;
  const FeeSchedule &fees_;
};

class LedgerAuditor {
public:
  LedgerAuditor(const AssetRegistry &assets, const OracleBook &oracles, const Ledger &ledger);

  AuditSummary summarize() const;
  void summarizeAccounts(AuditSummary &summary) const;
  void summarizeAssetTotals(AuditSummary &summary) const;

private:
  const AssetRegistry &assets_;
  const OracleBook &oracles_;
  const Ledger &ledger_;
};

class QuoteAuditor {
public:
  explicit QuoteAuditor(std::vector<QuoteResult> quotes);

  AuditSummary summarize() const;
  void summarizeAcceptance(AuditSummary &summary) const;
  void summarizeRoutes(AuditSummary &summary) const;

private:
  std::vector<QuoteResult> quotes_;
};

std::string auditItemsAsText(const AuditSummary &summary);

} // namespace aurora
