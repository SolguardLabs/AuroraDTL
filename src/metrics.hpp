#pragma once

#include "reconcile.hpp"

#include <map>
#include <string>
#include <vector>

namespace aurora {

struct MetricPoint {
  std::string name;
  std::string label;
  long double value{0.0L};
};

struct MetricSeries {
  std::string name;
  std::vector<MetricPoint> points;

  void add(std::string label, long double value);
  long double sum() const;
  long double max() const;
  long double min() const;
  long double average() const;
  std::uint64_t count() const;
};

struct MetricsSnapshot {
  std::vector<MetricSeries> series;

  void addPoint(const std::string &seriesName, const std::string &label, long double value);
  const MetricSeries *find(const std::string &seriesName) const;
  MetricSeries *findMutable(const std::string &seriesName);
};

struct MetricBucket {
  std::string label;
  long double lower{0.0L};
  long double upper{0.0L};
  std::uint64_t count{0};
};

class MetricHistogram {
public:
  void addBucket(std::string label, long double lower, long double upper);
  void observe(long double value);
  const std::vector<MetricBucket> &buckets() const;
  std::uint64_t overflow() const;
  std::uint64_t underflow() const;
  std::uint64_t observations() const;

private:
  std::vector<MetricBucket> buckets_;
  std::uint64_t overflow_{0};
  std::uint64_t underflow_{0};
  std::uint64_t observations_{0};
};

class MetricsCollector {
public:
  MetricsSnapshot collectQuotes(const std::vector<QuoteResult> &quotes) const;
  MetricsSnapshot collectSettlements(const std::vector<SettlementResult> &settlements) const;
  MetricsSnapshot collectReconciliation(const ReconcileReport &report) const;
  MetricsSnapshot collectLedger(const std::vector<LedgerEvent> &events) const;
  MetricsSnapshot merge(const std::vector<MetricsSnapshot> &snapshots) const;
  std::map<std::string, long double> totals(const MetricsSnapshot &snapshot) const;
  std::map<std::string, long double> averages(const MetricsSnapshot &snapshot) const;

private:
  static long double amountValue(Amount amount);
};

std::string metricSnapshotText(const MetricsSnapshot &snapshot);
std::vector<MetricPoint> flattenMetrics(const MetricsSnapshot &snapshot);
MetricSeries normalizeSeries(const MetricSeries &series);
MetricHistogram buildHistogram(const MetricSeries &series, std::uint64_t bucketCount);
std::string histogramText(const MetricHistogram &histogram);
std::uint64_t nonEmptyBuckets(const MetricHistogram &histogram);
std::uint64_t emptyBuckets(const MetricHistogram &histogram);

} // namespace aurora
