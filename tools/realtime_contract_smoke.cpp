#include "engine/SonRapTuneEngine.h"

#include <array>
#include <atomic>
#include <bitset>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

using namespace sonraptune;

namespace {

std::atomic<std::size_t> allocationCount{0};

bool finiteAndBounded(const std::vector<float>& values) noexcept
{
    for (const float value : values)
        if (!std::isfinite(value) || std::abs(value) > 2.0f)
            return false;
    return true;
}

} // namespace

void* operator new(std::size_t size)
{
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size))
        return memory;
    throw std::bad_alloc{};
}

void* operator new[](std::size_t size)
{
    allocationCount.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size))
        return memory;
    throw std::bad_alloc{};
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main()
{
    const std::array<double, 3> rates{44100.0, 48000.0, 96000.0};
    const std::array<int, 6> blocks{32, 64, 128, 256, 512, 1024};

    for (const double rate : rates) {
        for (const int block : blocks) {
            SonRapTuneEngine engine;
            engine.prepare({rate, block, 2});

            const int latency = engine.latencySamples();
            const int expectedLatency =
                std::max(16, static_cast<int>(std::lround(rate * 0.006)))
                + std::max(64, static_cast<int>(std::lround(rate * 0.020)));
            if (latency != expectedLatency || latency <= 0) {
                std::cerr << "FAIL latency rate=" << rate
                          << " block=" << block
                          << " expected=" << expectedLatency
                          << " actual=" << latency << '\n';
                return 2;
            }

            RuntimeParameters parameters;
            parameters.outputTrimDb = 0.0f;
            parameters.mix = 1.0f;
            parameters.bypass = true;
            engine.setParameters(parameters);

            const int renderSamples = latency + block * 3;
            std::vector<float> left(static_cast<std::size_t>(block), 0.0f);
            std::vector<float> right(static_cast<std::size_t>(block), 0.0f);
            std::vector<float> rendered(static_cast<std::size_t>(renderSamples), 0.0f);
            std::array<float*, 2> channels{left.data(), right.data()};
            std::bitset<128> midi;

            const std::size_t allocationsBeforeProcess =
                allocationCount.load(std::memory_order_relaxed);

            int renderedCount = 0;
            while (renderedCount < renderSamples) {
                const int count = std::min(block, renderSamples - renderedCount);
                std::fill(left.begin(), left.end(), 0.0f);
                std::fill(right.begin(), right.end(), 0.0f);
                if (renderedCount == 0) {
                    left[0] = 1.0f;
                    right[0] = 1.0f;
                }

                engine.process(channels.data(), 2, count, midi);
                for (int sample = 0; sample < count; ++sample)
                    rendered[static_cast<std::size_t>(renderedCount + sample)] =
                        left[static_cast<std::size_t>(sample)];
                renderedCount += count;
            }

            const std::size_t allocationsAfterProcess =
                allocationCount.load(std::memory_order_relaxed);
            if (allocationsAfterProcess != allocationsBeforeProcess) {
                std::cerr << "FAIL audio-thread allocation rate=" << rate
                          << " block=" << block
                          << " allocations="
                          << (allocationsAfterProcess - allocationsBeforeProcess)
                          << '\n';
                return 3;
            }

            if (!finiteAndBounded(rendered)) {
                std::cerr << "FAIL non-finite output rate=" << rate
                          << " block=" << block << '\n';
                return 4;
            }

            int peakIndex = 0;
            float peak = 0.0f;
            for (int sample = 0; sample < renderSamples; ++sample) {
                const float magnitude = std::abs(
                    rendered[static_cast<std::size_t>(sample)]);
                if (magnitude > peak) {
                    peak = magnitude;
                    peakIndex = sample;
                }
            }

            if (peakIndex != latency || std::abs(peak - 1.0f) > 0.0001f) {
                std::cerr << "FAIL bypass alignment rate=" << rate
                          << " block=" << block
                          << " expectedIndex=" << latency
                          << " actualIndex=" << peakIndex
                          << " peak=" << peak << '\n';
                return 5;
            }

            // Exercise the active detector/trajectory/shifter path after reset.
            engine.reset();
            parameters.bypass = false;
            parameters.tune = 1.0f;
            parameters.speedMs = 5.0f;
            parameters.stability = 0.8f;
            parameters.feel = 0.0f;
            engine.setParameters(parameters);
            midi.set(60);

            double phase = 0.0;
            for (int iteration = 0; iteration < 200; ++iteration) {
                for (int sample = 0; sample < block; ++sample) {
                    phase += 2.0 * 3.14159265358979323846 * 220.0 / rate;
                    if (phase >= 2.0 * 3.14159265358979323846)
                        phase -= 2.0 * 3.14159265358979323846;
                    const float value = 0.2f * static_cast<float>(std::sin(phase));
                    left[static_cast<std::size_t>(sample)] = value;
                    right[static_cast<std::size_t>(sample)] = value;
                }
                engine.process(channels.data(), 2, block, midi);
                if (!finiteAndBounded(left) || !finiteAndBounded(right)) {
                    std::cerr << "FAIL active output rate=" << rate
                              << " block=" << block << '\n';
                    return 6;
                }
            }
        }
    }

    std::cout << "realtime_contract_smoke=PASS rates=3 block_sizes=6 "
                 "fixed_latency=verified allocations_in_process=0\n";
    return 0;
}
