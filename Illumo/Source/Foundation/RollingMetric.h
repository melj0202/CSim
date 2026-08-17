#pragma once

#include <algorithm>
#include <array>
#include <cstddef>

class RollingMetric
{
public:
  static constexpr std::size_t kCapacity = 256u;

  void add(double value)
  {
    samples[nextIndex] = value;
    nextIndex = (nextIndex + 1u) % kCapacity;
    if (sampleCount < kCapacity) {
      sampleCount += 1u;
    }
  }

  double percentile(double fraction) const
  {
    if (sampleCount == 0u) {
      return 0.0;
    }
    std::array<double, kCapacity> sorted{};
    for (std::size_t index = 0u; index < sampleCount; ++index) {
      sorted[index] = samples[index];
    }
    std::sort(sorted.begin(), sorted.begin() + sampleCount);
    const double clamped = std::clamp(fraction, 0.0, 1.0);
    const std::size_t percentileIndex =
      static_cast<std::size_t>(clamped * static_cast<double>(sampleCount - 1u));
    return sorted[percentileIndex];
  }

  double median() const { return percentile(0.5); }
  double p95() const { return percentile(0.95); }
  double maximum() const { return percentile(1.0); }
  std::size_t size() const { return sampleCount; }

private:
  std::array<double, kCapacity> samples{};
  std::size_t nextIndex = 0u;
  std::size_t sampleCount = 0u;
};
