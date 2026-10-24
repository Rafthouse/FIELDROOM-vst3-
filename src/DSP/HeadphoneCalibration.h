#pragma once

#include <vector>
#include <string>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
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

/**
 * Headphone correction (PEQ + automatic preamp) for the audio thread.
 *
 * Threading model:
 *  - prepare() (message thread, before playback) builds one immutable PreparedCalibrationState per
 *    bundled profile. Nothing is computed or allocated on the audio thread afterwards.
 *  - requestState() (any thread) packs the whole request into one 64-bit atomic. Reads and writes are
 *    therefore always consistent, with no locks and no allocation.
 *  - process() (audio thread) picks up a changed request at a block boundary, builds it into the idle
 *    filter bank from the table, and crossfades to it. A new request arriving mid-fade is applied after
 *    that fade completes, so a fade is never interrupted.
 */
class HeadphoneCalibration {
public:
    static constexpr int MaxBands = 10;

    // Fixed-size POD: copying it into a filter bank never allocates
    struct PreparedCalibrationState {
        int profileIndex{-1};
        CalibrationStatus status{CalibrationStatus::UncalibratedRaw};
        bool calibrationEnabled{true};
        bool hasIndependentLR{false};
        float autoPreampGainL{1.0f}; // linear, from the profile's default preamp
        float autoPreampGainR{1.0f};
        float targetPreampGainL{1.0f}; // linear, applied for the current request
        float targetPreampGainR{1.0f};
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

    // Message thread, before playback: builds the profile table and the initial filter bank
    void prepare(double sampleRate, int maxBlockSize);
    void reset() noexcept;

    // Any thread: lock-free request, applied by process() at the next idle moment
    void requestState(int profileIndex, bool enabled, bool autoPreamp, float manualGainDb) noexcept;
    void requestStateById(const std::string& profileId, bool enabled, bool autoPreamp, float manualGainDb);

    // Audio thread: real-time safe, no allocation, no locks
    void process(float* leftChannel, float* rightChannel, int numSamples) noexcept;

    static std::vector<HeadphoneProfile> getBundledProfiles();
    static float calculateAutoPreamp(const std::vector<BiquadBandConfig>& bands);
    static BiquadCoeffs calculateBiquadCoeffs(const BiquadBandConfig& band, double sampleRate);

    int getProfileIndexById(const std::string& id) const;
    std::string getProfileIdByIndex(int index) const;

private:
    struct Request {
        int profileIndex{0};
        bool enabled{true};
        bool autoPreamp{true};
        float manualGainDb{0.0f};
    };

    static uint64_t packRequest(const Request& r) noexcept;
    static Request unpackRequest(uint64_t packed) noexcept;

    PreparedCalibrationState makeTableEntry(int profileIndex) const;
    void buildBank(FilterBank& bank, const Request& req, const FilterBank* crossfadeFrom) noexcept;

    static constexpr int CrossfadeSamples = 512; // ~10.7 ms at 48 kHz

    std::vector<HeadphoneProfile> bundledProfiles;   // message thread metadata
    std::vector<PreparedCalibrationState> table;      // immutable after prepare()

    std::atomic<uint64_t> requested{0};              // written by any thread
    uint64_t applied{0};                             // audio thread only

    double currentSampleRate{48000.0};
    int maxBlockSizeSamples{512};

    FilterBank banks[2]{};
    int activeBank{0};
    int pendingBank{1};
    bool isCrossfading{false};
    float crossfadeProgress{0.0f};
    float crossfadeStep{1.0f / static_cast<float>(CrossfadeSamples)};
    float gainSmoothingCoeff{0.005f}; // Smooth preamp transitions
};

} // namespace fieldroom
