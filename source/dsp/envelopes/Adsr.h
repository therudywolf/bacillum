#pragma once

#include <algorithm>

namespace bacillum::dsp
{
    // Linear ADSR with soft-ramp retrigger to avoid clicks when noteOn
    // arrives while the previous voice is still releasing. Per-sample tick.
    // Spec §5.4.1.
    class AdsrLinear
    {
    public:
        enum class Stage { Idle, Attack, Decay, Sustain, Release };

        void prepare(double sampleRate) noexcept
        {
            sr = static_cast<float>(sampleRate);
            reset();
        }

        void reset() noexcept
        {
            stage = Stage::Idle;
            level = 0.0f;
            releaseFrom = 0.0f;
            updateIncrements();
        }

        void setAttack (float seconds) noexcept { aSec = std::max(0.0001f, seconds); updateIncrements(); }
        void setDecay  (float seconds) noexcept { dSec = std::max(0.0001f, seconds); updateIncrements(); }
        void setSustain(float linear)  noexcept { sLvl = std::clamp(linear, 0.0f, 1.0f); }
        void setRelease(float seconds) noexcept { rSec = std::max(0.0001f, seconds); updateIncrements(); }

        void noteOn() noexcept
        {
            // soft retrigger: start from current level instead of 0
            stage = Stage::Attack;
            // Recompute attack increment so we still reach 1.0 in aSec from wherever we are.
            const float remaining = std::max(0.0001f, 1.0f - level);
            aInc = remaining / (aSec * sr);
        }

        void noteOff() noexcept
        {
            if (stage == Stage::Idle) return;
            releaseFrom = level;
            stage = Stage::Release;
            rInc = releaseFrom / (rSec * sr);
        }

        // Hard kill, used by voice stealing when no glitch budget exists.
        void killFast() noexcept
        {
            stage = Stage::Release;
            releaseFrom = level;
            rInc = level / std::max(1.0f, 0.005f * sr);  // 5ms fade
        }

        [[nodiscard]] bool isActive() const noexcept { return stage != Stage::Idle; }
        [[nodiscard]] bool isInRelease() const noexcept { return stage == Stage::Release; }
        [[nodiscard]] float getLevel() const noexcept { return level; }
        [[nodiscard]] Stage getStage() const noexcept { return stage; }

        [[nodiscard]] float tick() noexcept
        {
            switch (stage)
            {
                case Stage::Idle:
                    return 0.0f;

                case Stage::Attack:
                    level += aInc;
                    if (level >= 1.0f)
                    {
                        level = 1.0f;
                        stage = Stage::Decay;
                        dInc = (1.0f - sLvl) / (dSec * sr);
                    }
                    break;

                case Stage::Decay:
                    level -= dInc;
                    if (level <= sLvl)
                    {
                        level = sLvl;
                        stage = Stage::Sustain;
                    }
                    break;

                case Stage::Sustain:
                    level = sLvl;
                    break;

                case Stage::Release:
                    level -= rInc;
                    if (level <= 0.0f)
                    {
                        level = 0.0f;
                        stage = Stage::Idle;
                    }
                    break;
            }
            return level;
        }

    private:
        void updateIncrements() noexcept
        {
            aInc = 1.0f / (aSec * sr);
            dInc = (1.0f - sLvl) / (dSec * sr);
            rInc = (releaseFrom > 0.0f ? releaseFrom : 1.0f) / (rSec * sr);
        }

        float sr   { 48000.0f };
        float aSec { 0.005f };
        float dSec { 0.20f };
        float sLvl { 0.7f };
        float rSec { 0.40f };

        float aInc { 0.0f };
        float dInc { 0.0f };
        float rInc { 0.0f };

        float level       { 0.0f };
        float releaseFrom { 0.0f };
        Stage stage       { Stage::Idle };
    };
}
