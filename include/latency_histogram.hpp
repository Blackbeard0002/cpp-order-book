#pragma once

#include <array>
#include <cstdint>
#include <ostream>

class LatencyHistogram {
public:
    static constexpr std::size_t kNumBuckets = 32;

    void record(std::uint64_t ns) noexcept {
        const std::size_t idx = (ns == 0) ? 0 : 63 - __builtin_clzll(ns);
        const std::size_t clamped = (idx < kNumBuckets) ? idx : kNumBuckets - 1;
        ++buckets_[clamped];
        ++count_;
        total_ns_ += ns;
    }

    void reset() noexcept {
        buckets_.fill(0);
        count_ = 0;
        total_ns_ = 0;
    }

    [[nodiscard]] std::uint64_t count() const noexcept { return count_; }

    [[nodiscard]] std::uint64_t percentile(double q) const noexcept {
        if (count_ == 0) {
            return 0;
        }
        auto target = static_cast<std::uint64_t>(static_cast<double>(count_) * q);
        if (target == 0) {
            target = 1;
        }
        std::uint64_t cum = 0;
        for (std::size_t i = 0; i < kNumBuckets; ++i) {
            cum += buckets_[i];
            if (cum >= target) {
                return 1ULL << (i + 1);
            }
        }
        return 1ULL << kNumBuckets;
    }

    [[nodiscard]] double mean_ns() const noexcept {
        return count_ == 0 ? 0.0 : static_cast<double>(total_ns_) / static_cast<double>(count_);
    }

    void print(std::ostream& os) const {
        os << "Latency (per add_order, ns):\n"
           << "  count  = " << count_ << '\n'
           << "  mean   = " << static_cast<std::uint64_t>(mean_ns()) << " ns\n"
           << "  p50    <= " << percentile(0.50) << " ns\n"
           << "  p90    <= " << percentile(0.90) << " ns\n"
           << "  p95    <= " << percentile(0.95) << " ns\n"
           << "  p99    <= " << percentile(0.99) << " ns\n"
           << "  p99.9  <= " << percentile(0.999) << " ns\n"
           << "  p99.99 <= " << percentile(0.9999) << " ns\n";
    }

private:
    std::array<std::uint64_t, kNumBuckets> buckets_{};
    std::uint64_t count_ = 0;
    std::uint64_t total_ns_ = 0;
};
