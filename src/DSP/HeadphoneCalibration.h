#pragma once

#include <vector>
#include <string>
#include <array>
#include <atomic>
#include <cmath>
#include <juce_core/juce_core.h>

namespace fieldroom {

enum class FilterType {
    Peaking = 0,
    LowShelf = 1,
    HighShelf = 2,
    LowPass = 3,
    HighPass = 4
};

enum class CalibrationStatus : int {
    VerifiedCalibrated = 0,
    CalibrationPending = 1,
    UncalibratedRaw = 2
};

inline const char* getCalibrationStatusString(CalibrationStatus s) noexcept {
    switch (s) {
        case CalibrationStatus::CalibrationPending: return "CALIBRATION_PENDING";
        case CalibrationStatus::VerifiedCalibrated: return "VERIFIED_CALIBRATED";
        case CalibrationStatus::UncalibratedRaw: return "UNCALIBRATED_RAW";
    }
    return "UNCALIBRATED_RAW";
}

inline bool isCalibrationPending(CalibrationStatus s) noexcept {
    return s == CalibrationStatus::CalibrationPending;
}

inline CalibrationStatus parseCalibrationStatus(const std::string& str) {
    if (str == "CALIBRATION_PENDING" || str == "CalibrationPending") return CalibrationStatus::CalibrationPending;
    if (str == "VERIFIED_CALIBRATED" || str == "VerifiedCalibrated") return CalibrationStatus::VerifiedCalibrated;
    return CalibrationStatus::UncalibratedRaw;
}

struct BiquadBandConfig {
    FilterType type{FilterType::Peaking};
    float freqHz{1000.0f};
    float gainDb{0.0f};
    float q{1.0f};
};

struct HeadphoneProfile {
    std::string id;
    std::string manufacturer;
    std::string model;
    std::string measurementSource;
    std::string measurementRig;
    std::string targetCurve;
    CalibrationStatus status{CalibrationStatus::VerifiedCalibrated};
    bool hasIndependentLR{false};
    float defaultPreampLeftDb{0.0f};
    float defaultPreampRightDb{0.0f};
    std::vector<BiquadBandConfig> bandsLeft;
    std::vector<BiquadBandConfig> bandsRight;
};

// Raw biquad coefficients: y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]
struct BiquadCoeffs {
    float b0{1.0f};
    float b1{0.0f};
    float b2{0.0f};
    float a1{0.0f};
    float a2{0.0f};
};

// Direct Form II Transposed filter state
struct BiquadState {
    float s1{0.0f};
    float s2{0.0f};

    inline void reset() noexcept {
        s1 = 0.0f;
        s2 = 0.0f;
    }

    inline float process(float in, const BiquadCoeffs& c) noexcept {
        float out = c.b0 * in + s1;
        s1 = c.b1 * in - c.a1 * out + s2;
        s2 = c.b2 * in - c.a2 * out;
        return out;
    }
};

class HeadphoneCalibration {
public:
    static constexpr int MaxBands = 10;

    // Fully self-contained, fixed-size POD struct for real-time safety (no heap allocations)
    struct PreparedCalibrationState {
        char profileId[64]{0};
        int profileIndex{-1};
        CalibrationStatus status{CalibrationStatus::UncalibratedRaw};
        bool calibrationEnabled{true};
        bool hasIndependentLR{false};
        float targetPreampGainL{1.0f}; // linear gain
        float targetPreampGainR{1.0f}; // linear gain
        int numBandsL{0};
        int numBandsR{0};
        std::array<BiquadCoeffs, MaxBands> coeffsL{};
        std::array<BiquadCoeffs, MaxBands> coeffsR{};
    };

    struct FilterBank {
        PreparedCalibrationState state{};
        std::array<BiquadState, MaxBands> bandsL{};
        std::array<BiquadState, MaxBands> bandsR{};
        float currentPreampL{1.0f};
        float currentPreampR{1.0f};

        void reset() noexcept {
            for (auto& b : bandsL) b.reset();
            for (auto& b : bandsR) b.reset();
        }

        void process(float inL, float inR, float& outL, float& outR, float smoothingCoeff) noexcept {
            currentPreampL += smoothingCoeff * (state.targetPreampGainL - currentPreampL);
            currentPreampR += smoothingCoeff * (state.targetPreampGainR - currentPreampR);

            if (!state.calibrationEnabled || state.status != CalibrationStatus::VerifiedCalibrated) {
                outL = inL;
                outR = inR;
                return;
            }

            float l = inL * currentPreampL;
            float r = inR * currentPreampR;

            for (int i = 0; i < state.numBandsL; ++i) {
                l = bandsL[static_cast<size_t>(i)].process(l, state.coeffsL[static_cast<size_t>(i)]);
            }

            for (int i = 0; i < state.numBandsR; ++i) {
                r = bandsR[static_cast<size_t>(i)].process(r, state.coeffsR[static_cast<size_t>(i)]);
            }

            outL = l;
            outR = r;
        }
    };

    HeadphoneCalibration();
    ~HeadphoneCalibration() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    // Configuration methods intended to be called OUTSIDE the audio thread (e.g. APVTS listener, UI, or prepareToPlay)
    void prepareAndPublishState(int profileIdx, bool enabled, bool autoPreamp, float manualGainDb);
    void prepareAndPublishStateById(const std::string& profileId, bool enabled, bool autoPreamp, float manualGainDb);

    // Real-time audio thread callback (deterministic O(N), zero locks, zero allocations)
    void process(float* leftChannel, float* rightChannel, int numSamples) noexcept;

    static std::vector<HeadphoneProfile> getBundledProfiles();
    static float calculateAutoPreamp(const std::vector<BiquadBandConfig>& bands);
    static BiquadCoeffs calculateBiquadCoeffs(const BiquadBandConfig& band, double sampleRate);

    const HeadphoneProfile& getCurrentProfile() const noexcept { return currentProfile; }
    int getCurrentProfileIndex() const noexcept { return lastPreparedProfileIndex; }
    bool isEnabled() const noexcept { return lastPreparedEnabled; }
    float getEffectivePreampLeftDb() const noexcept { return lastEffectivePreampLeftDb; }
    float getEffectivePreampRightDb() const noexcept { return lastEffectivePreampRightDb; }

    int getProfileIndexById(const std::string& id) const;
    std::string getProfileIdByIndex(int index) const;

private:
    double currentSampleRate{48000.0};
    int maxBlockSizeSamples{512};
    std::vector<HeadphoneProfile> bundledProfiles;
    HeadphoneProfile currentProfile;

    // Cache of last prepared parameters for quick retrieval
    int lastPreparedProfileIndex{0};
    bool lastPreparedEnabled{true};
    bool lastPreparedAutoPreamp{true};
    float lastPreparedManualGainDb{0.0f};
    float lastEffectivePreampLeftDb{0.0f};
    float lastEffectivePreampRightDb{0.0f};

    // Lock-free Triple Buffer for asynchronous state publication
    PreparedCalibrationState statePool[3]{};
    int writeIndex{2};
    std::atomic<int> sharedIndex{1};
    int readIndex{0};
    std::atomic<bool> hasNewState{false};

    // Dual-bank crossfading system for click-free switching on the audio thread
    FilterBank banks[2]{};
    int activeBank{0};
    int pendingBank{1};
    bool isCrossfading{false};
    float crossfadeProgress{0.0f};
    float crossfadeStep{0.002f}; // ~500 samples crossfade (~10.4ms at 48kHz)
    float gainSmoothingCoeff{0.005f}; // Smooth preamp transitions

    void activateNewState(const PreparedCalibrationState& newState) noexcept;
};

} // namespace fieldroom
