#pragma once

#include "engine/CommonTypes.h"
#include <array>
#include <atomic>

namespace sonraptune {

class PitchTelemetryTap {
public:
    void push(const PitchFrame& frame) noexcept
    {
        const auto write = write_.load(std::memory_order_relaxed);
        const auto next = (write + 1u) & (kCapacity - 1u);
        if (next == read_.load(std::memory_order_acquire))
            read_.store((read_.load(std::memory_order_relaxed) + 1u) & (kCapacity - 1u),
                        std::memory_order_release);
        data_[write] = frame;
        write_.store(next, std::memory_order_release);
    }

    bool pop(PitchFrame& frame) noexcept
    {
        const auto read = read_.load(std::memory_order_relaxed);
        if (read == write_.load(std::memory_order_acquire)) return false;
        frame = data_[read];
        read_.store((read + 1u) & (kCapacity - 1u), std::memory_order_release);
        return true;
    }

private:
    static constexpr std::size_t kCapacity = 256;
    static_assert((kCapacity & (kCapacity - 1u)) == 0u);
    std::array<PitchFrame, kCapacity> data_{};
    std::atomic<std::size_t> write_{0};
    std::atomic<std::size_t> read_{0};
};

} // namespace sonraptune
