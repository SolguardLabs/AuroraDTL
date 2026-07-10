#include "metrics.hpp"

#include <sstream>

namespace aurora {

void MetricSeries::add(std::string label, long double value) {
  points.push_back(MetricPoint{name, std::move(label), value});
}

long double MetricSeries::sum() const {
  long double total = 0.0L;
  for (const MetricPoint &point : points) {
    total += point.value;
  }
  return total;
}

long double MetricSeries::max() const {
  if (points.empty()) {
    return 0.0L;
  }
  long double result = points.front().value;
  for (const MetricPoint &point : points) {
    result = std::max(result, point.value);
  }
  return result;
}

long double MetricSeries::min() const {
  if (points.empty()) {
    return 0.0L;
  }
  long double result = points.front().value;
  for (const MetricPoint &point : points) {
    result = std::min(result, point.value);
  }
  return result;
}

long double MetricSeries::average() const {
  if (points.empty()) {
    return 0.0L;
  }
  return sum() / static_cast<long double>(points.size());
}

std::uint64_t MetricSeries::count() const {
  return static_cast<std::uint64_t>(points.size());
}

void MetricsSnapshot::addPoint(const std::string &seriesName, const std::string &label, long double value) {
  MetricSeries *targetSeries = findMutable(seriesName);
  if (targetSeries == nullptr) {
    MetricSeries created;
    created.name = seriesName;
    this->series.push_back(created);
    targetSeries = &this->series.back();
  }
  targetSeries->add(label, value);
}

const MetricSeries *MetricsSnapshot::find(const std::string &seriesName) const {
  for (const MetricSeries &item : series) {
    if (item.name == seriesName) {
      return &item;
    }
  }
  return nullptr;
}

MetricSeries *MetricsSnapshot::findMutable(const std::string &seriesName) {
  for (MetricSeries &item : series) {
    if (item.name == seriesName) {
      return &item;
    }
  }
  return nullptr;
}

void MetricHistogram::addBucket(std::string label, long double lower, long double upper) {
  require(upper >= lower, "metric histogram bucket has inverted bounds");
  buckets_.push_back(MetricBucket{std::move(label), lower, upper, 0U});
}

void MetricHistogram::observe(long double value) {
  ++observations_;
  if (buckets_.empty()) {
    ++overflow_;
    return;
  }
  if (value < buckets_.front().lower) {
    ++underflow_;
    return;
  }
  for (MetricBucket &bucket : buckets_) {
    if (value >= bucket.lower && value <= bucket.upper) {
      ++bucket.count;
      return;
    }
  }
  ++overflow_;
}

const std::vector<MetricBucket> &MetricHistogram::buckets() const {
  return buckets_;
}

std::uint64_t MetricHistogram::overflow() const {
  return overflow_;
}

std::uint64_t MetricHistogram::underflow() const {
  return underflow_;
}

std::uint64_t MetricHistogram::observations() const {
  return observations_;
}

MetricsSnapshot MetricsCollector::collectQuotes(const std::vector<QuoteResult> &quotes) const {
  MetricsSnapshot snapshot;
  for (const QuoteResult &quote : quotes) {
    snapshot.addPoint("quote.gross", quote.id, amountValue(quote.grossOut));
    snapshot.addPoint("quote.net", quote.id, amountValue(quote.netOut));
    snapshot.addPoint("quote.fee", quote.id, amountValue(quote.fees.fee));
    snapshot.addPoint("quote.accepted", quote.reason, quote.accepted ? 1.0L : 0.0L);
  }
  return snapshot;
}

MetricsSnapshot MetricsCollector::collectSettlements(const std::vector<SettlementResult> &settlements) const {
  MetricsSnapshot snapshot;
  for (const SettlementResult &settlement : settlements) {
    snapshot.addPoint("settlement.sourceDebit", settlement.id, amountValue(settlement.sourceDebit));
    snapshot.addPoint("settlement.destinationCredit", settlement.id, amountValue(settlement.destinationCredit));
    snapshot.addPoint("settlement.fee", settlement.id, amountValue(settlement.destinationFee));
    snapshot.addPoint("settlement.status", settlement.status, settlement.status == "settled" ? 1.0L : 0.0L);
  }
  return snapshot;
}

MetricsSnapshot MetricsCollector::collectReconciliation(const ReconcileReport &report) const {
  MetricsSnapshot snapshot;
  for (const AssetTotal &total : report.assetTotals) {
    snapshot.addPoint("reconcile.asset.debits", total.asset, amountValue(total.debits));
    snapshot.addPoint("reconcile.asset.credits", total.asset, amountValue(total.credits));
    snapshot.addPoint("reconcile.asset.net", total.asset, amountValue(total.net));
  }
  for (const SettlementAggregate &aggregate : report.settlementAggregates) {
    snapshot.addPoint("reconcile.route.settled", aggregate.routeId, static_cast<long double>(aggregate.settled));
    snapshot.addPoint("reconcile.route.rejected", aggregate.routeId, static_cast<long double>(aggregate.rejected));
  }
  snapshot.addPoint("reconcile.findings", "count", static_cast<long double>(report.findings.size()));
  return snapshot;
}

MetricsSnapshot MetricsCollector::collectLedger(const std::vector<LedgerEvent> &events) const {
  MetricsSnapshot snapshot;
  for (const LedgerEvent &event : events) {
    const long double value = amountValue(event.amount);
    snapshot.addPoint("ledger." + event.type, event.asset, value);
    snapshot.addPoint("ledger.events", event.type, 1.0L);
  }
  return snapshot;
}

MetricsSnapshot MetricsCollector::merge(const std::vector<MetricsSnapshot> &snapshots) const {
  MetricsSnapshot merged;
  for (const MetricsSnapshot &snapshot : snapshots) {
    for (const MetricSeries &series : snapshot.series) {
      for (const MetricPoint &point : series.points) {
        merged.addPoint(series.name, point.label, point.value);
      }
    }
  }
  return merged;
}

std::map<std::string, long double> MetricsCollector::totals(const MetricsSnapshot &snapshot) const {
  std::map<std::string, long double> result;
  for (const MetricSeries &item : snapshot.series) {
    result[item.name] = item.sum();
  }
  return result;
}

std::map<std::string, long double> MetricsCollector::averages(const MetricsSnapshot &snapshot) const {
  std::map<std::string, long double> result;
  for (const MetricSeries &item : snapshot.series) {
    result[item.name] = item.average();
  }
  return result;
}

long double MetricsCollector::amountValue(Amount amount) {
  return static_cast<long double>(amount.raw());
}

std::string metricSnapshotText(const MetricsSnapshot &snapshot) {
  std::ostringstream out;
  for (const MetricSeries &series : snapshot.series) {
    out << series.name << ".sum=" << OracleBook::formatPrice(series.sum()) << "\n";
    out << series.name << ".min=" << OracleBook::formatPrice(series.min()) << "\n";
    out << series.name << ".max=" << OracleBook::formatPrice(series.max()) << "\n";
    out << series.name << ".avg=" << OracleBook::formatPrice(series.average()) << "\n";
    out << series.name << ".count=" << series.count() << "\n";
  }
  return out.str();
}

std::vector<MetricPoint> flattenMetrics(const MetricsSnapshot &snapshot) {
  std::vector<MetricPoint> result;
  for (const MetricSeries &item : snapshot.series) {
    result.insert(result.end(), item.points.begin(), item.points.end());
  }
  return result;
}

MetricSeries normalizeSeries(const MetricSeries &series) {
  MetricSeries normalized;
  normalized.name = series.name;
  const long double high = series.max();
  if (high <= 0.0L) {
    return normalized;
  }
  for (const MetricPoint &point : series.points) {
    normalized.add(point.label, point.value / high);
  }
  return normalized;
}

MetricHistogram buildHistogram(const MetricSeries &series, std::uint64_t bucketCount) {
  MetricHistogram histogram;
  if (bucketCount == 0U || series.points.empty()) {
    return histogram;
  }
  const long double low = series.min();
  const long double high = series.max();
  const long double width = high > low ? (high - low) / static_cast<long double>(bucketCount) : 1.0L;
  for (std::uint64_t i = 0; i < bucketCount; ++i) {
    const long double lower = low + (width * static_cast<long double>(i));
    const long double upper = i + 1U == bucketCount ? high : lower + width;
    histogram.addBucket("b" + std::to_string(i), lower, upper);
  }
  for (const MetricPoint &point : series.points) {
    histogram.observe(point.value);
  }
  return histogram;
}

std::string histogramText(const MetricHistogram &histogram) {
  std::ostringstream out;
  for (const MetricBucket &bucket : histogram.buckets()) {
    out << bucket.label << "=" << bucket.count << "\n";
  }
  out << "underflow=" << histogram.underflow() << "\n";
  out << "overflow=" << histogram.overflow() << "\n";
  out << "observations=" << histogram.observations() << "\n";
  return out.str();
}

std::uint64_t nonEmptyBuckets(const MetricHistogram &histogram) {
  std::uint64_t count = 0U;
  for (const MetricBucket &bucket : histogram.buckets()) {
    if (bucket.count != 0U) {
      ++count;
    }
  }
  return count;
}

std::uint64_t emptyBuckets(const MetricHistogram &histogram) {
  const std::uint64_t total = static_cast<std::uint64_t>(histogram.buckets().size());
  const std::uint64_t used = nonEmptyBuckets(histogram);
  return total >= used ? total - used : 0U;
}

} // namespace aurora
