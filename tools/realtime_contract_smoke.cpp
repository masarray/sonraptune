#include "engine/SonRapTuneEngine.h"

#include <array>
#include <bitset>
#include <cmath>
#include <iostream>
#include <vector>

using namespace sonraptune;

int main()
{
    const std::array<double, 3> rates{44100.0, 48000.0, 96000.0};
    const std::array<int, 6> blocks{32, 64, 128, 256, 512, 1024};
    for (double rate : rates) {
        for (int block : blocks) {
            SonRapTuneEngine engine;
            engine.prepare({rate, block, 2});
            RuntimeParameters p;
            p.outputTrimDb = 0.0f;
            engine.setParameters(p);
            std::vector<float> left(static_cast<std::size_t>(block));
            std::vector<float> right(static_cast<std::size_t>(block));
            std::array<float*, 2> channels{left.data(), right.data()};
            std::bitset<128> midi;
            midi.set(60);
            double phase = 0.0;
            for (int iteration = 0; iteration < 400; ++iteration) {
                for (int i = 0; i < block; ++i) {
                    phase += 2.0 * 3.14159265358979323846 * 220.0 / rate;
                    if (phase > 2.0 * 3.14159265358979323846) phase -= 2.0 * 3.14159265358979323846;
                    left[static_cast<std::size_t>(i)] = 0.2f * std::sin(phase);
                    right[static_cast<std::size_t>(i)] = left[static_cast<std::size_t>(i)];
                }
                engine.process(channels.data(), 2, block, midi);
                for (int i = 0; i < block; ++i) {
                    if (!std::isfinite(left[static_cast<std::size_t>(i)]) ||
                        !std::isfinite(right[static_cast<std::size_t>(i)])) return 2;
                }
            }
            if (engine.latencySamples() != 0) return 3;
        }
    }
    std::cout << "realtime_contract_smoke=PASS rates=3 block_sizes=6\n";
    return 0;
}
