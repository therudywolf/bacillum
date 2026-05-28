#pragma once

#include <atomic>
#include <array>
#include <algorithm>

namespace bacillum::dsp
{
    // Lock-free SPSC ring buffer for audio→UI visualisation.
    //
    // Audio thread pushes mono-mixed samples; UI Timer reads the most recent
    // N samples without locking. Memory ordering on the write index sequences
    // sample writes before the read.
    //
    // Capacity is a compile-time power of two so we mask instead of modulo.
    class AudioVizBuffer
    {
    public:
        static constexpr int kCapacity = 16384;    // ~340 ms @ 48 kHz — plenty for 2048-pt FFT + scope window
        static_assert ((kCapacity & (kCapacity - 1)) == 0, "kCapacity must be power of two");

        // Reset everything (Message thread, before audio starts).
        void reset() noexcept
        {
            buffer.fill (0.0f);
            writePos.store (0, std::memory_order_release);
        }

        // Audio thread. Pushes mono mix of L/R into the ring.
        void push (const float* L, const float* R, int numSamples) noexcept
        {
            const int start = writePos.load (std::memory_order_relaxed);
            for (int i = 0; i < numSamples; ++i)
            {
                const float s = 0.5f * (L[i] + R[i]);
                buffer[(start + i) & (kCapacity - 1)] = s;
            }
            writePos.store (start + numSamples, std::memory_order_release);
        }

        // UI thread. Copies the most recent `n` samples (in chronological
        // order) into `out`. n must be ≤ kCapacity. Returns true if there
        // was enough history; false means the ring hasn't filled yet.
        bool readLatest (float* out, int n) const noexcept
        {
            const int wp = writePos.load (std::memory_order_acquire);
            if (n > kCapacity || wp < n) return false;
            const int from = wp - n;
            for (int i = 0; i < n; ++i)
                out[i] = buffer[(from + i) & (kCapacity - 1)];
            return true;
        }

        // Used by oscilloscope to look further back from the most recent
        // sample for trigger search.
        int getWritePos() const noexcept { return writePos.load (std::memory_order_acquire); }

        float at (int absoluteIndex) const noexcept
        {
            return buffer[absoluteIndex & (kCapacity - 1)];
        }

    private:
        // mutable not needed — UI never modifies.
        std::array<float, kCapacity> buffer {};
        std::atomic<int> writePos { 0 };
    };
}
