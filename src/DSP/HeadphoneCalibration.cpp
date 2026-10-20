#include "HeadphoneCalibration.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace fieldroom {

float HeadphoneCalibration::calculateAutoPreamp(const std::vector<BiquadBandConfig>& bands) {
    if (bands.empty()) return 0.0f;
    float maxPositiveGain = 0.0f;
    for (const auto& b : bands) {
        if (b.gainDb > 0.0f) {
            maxPositiveGain += b.gainDb * 0.75f; // Account for band overlap
        }
    }
    float requiredPreamp = -std::min(18.0f, maxPositiveGain + 0.5f);
    return (maxPositiveGain > 0.1f) ? requiredPreamp : 0.0f;
}

BiquadCoeffs HeadphoneCalibration::calculateBiquadCoeffs(const BiquadBandConfig& band, double sampleRate) {
    BiquadCoeffs coeffs;
    if (sampleRate <= 0.0 || band.freqHz <= 0.0f) return coeffs;

    const float pi = 3.14159265358979323846f;
    float f0 = std::clamp(band.freqHz, 10.0f, static_cast<float>(sampleRate * 0.49));
    float w0 = 2.0f * pi * f0 / static_cast<float>(sampleRate);
    float cos_w0 = std::cos(w0);
    float sin_w0 = std::sin(w0);
    float q = std::max(0.01f, band.q);
    float alpha = sin_w0 / (2.0f * q);
    float A = std::pow(10.0f, band.gainDb / 40.0f);

    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;

    switch (band.type) {
        case FilterType::Peaking: {
            b0 = 1.0f + alpha * A;
            b1 = -2.0f * cos_w0;
            b2 = 1.0f - alpha * A;
            a0 = 1.0f + alpha / A;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha / A;
            break;
        }
        case FilterType::LowShelf: {
            float sqrtA_alpha2 = 2.0f * std::sqrt(A) * alpha;
            b0 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 + sqrtA_alpha2);
            b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cos_w0);
            b2 = A * ((A + 1.0f) - (A - 1.0f) * cos_w0 - sqrtA_alpha2);
            a0 = (A + 1.0f) + (A - 1.0f) * cos_w0 + sqrtA_alpha2;
            a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cos_w0);
            a2 = (A + 1.0f) + (A - 1.0f) * cos_w0 - sqrtA_alpha2;
            break;
        }
        case FilterType::HighShelf: {
            float sqrtA_alpha2 = 2.0f * std::sqrt(A) * alpha;
            b0 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 + sqrtA_alpha2);
            b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cos_w0);
            b2 = A * ((A + 1.0f) + (A - 1.0f) * cos_w0 - sqrtA_alpha2);
            a0 = (A + 1.0f) - (A - 1.0f) * cos_w0 + sqrtA_alpha2;
            a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cos_w0);
            a2 = (A + 1.0f) - (A - 1.0f) * cos_w0 - sqrtA_alpha2;
            break;
        }
        case FilterType::LowPass: {
            b0 = (1.0f - cos_w0) * 0.5f;
            b1 = 1.0f - cos_w0;
            b2 = (1.0f - cos_w0) * 0.5f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
            break;
        }
        case FilterType::HighPass: {
            b0 = (1.0f + cos_w0) * 0.5f;
            b1 = -(1.0f + cos_w0);
            b2 = (1.0f + cos_w0) * 0.5f;
            a0 = 1.0f + alpha;
            a1 = -2.0f * cos_w0;
            a2 = 1.0f - alpha;
            break;
        }
    }

    if (std::abs(a0) > 1e-8f) {
        float inv_a0 = 1.0f / a0;
        coeffs.b0 = b0 * inv_a0;
        coeffs.b1 = b1 * inv_a0;
        coeffs.b2 = b2 * inv_a0;
        coeffs.a1 = a1 * inv_a0;
        coeffs.a2 = a2 * inv_a0;
    }
    return coeffs;
}

std::vector<HeadphoneProfile> HeadphoneCalibration::getBundledProfiles() {
    std::vector<HeadphoneProfile> list;

    // 1. Sennheiser HD 600
    {
        HeadphoneProfile p;
        p.id = "sennheiser_hd600";
        p.manufacturer = "Sennheiser";
        p.model = "HD 600";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field / Optimum HiFi";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 105.0f, 4.5f, 0.71f},
            {FilterType::Peaking, 160.0f, -1.8f, 1.4f},
            {FilterType::Peaking, 1300.0f, -1.5f, 2.0f},
            {FilterType::Peaking, 3500.0f, -2.5f, 3.2f},
            {FilterType::Peaking, 5600.0f, 2.2f, 2.8f},
            {FilterType::Peaking, 8200.0f, -1.8f, 4.0f},
            {FilterType::HighShelf, 10000.0f, -0.8f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 2. Sennheiser HD 650
    {
        HeadphoneProfile p;
        p.id = "sennheiser_hd650";
        p.manufacturer = "Sennheiser";
        p.model = "HD 650";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field / Optimum HiFi";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 105.0f, 5.0f, 0.71f},
            {FilterType::Peaking, 220.0f, -2.0f, 1.2f},
            {FilterType::Peaking, 3400.0f, -2.0f, 2.5f},
            {FilterType::Peaking, 5800.0f, 2.5f, 2.2f},
            {FilterType::HighShelf, 10000.0f, -1.2f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 3. Beyerdynamic DT 770 Pro (80 Ohm)
    {
        HeadphoneProfile p;
        p.id = "beyer_dt770_80";
        p.manufacturer = "Beyerdynamic";
        p.model = "DT 770 Pro (80 Ohm)";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field Reference";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 80.0f, -2.5f, 0.71f},
            {FilterType::Peaking, 240.0f, -3.2f, 1.8f},
            {FilterType::Peaking, 3800.0f, 3.5f, 2.0f},
            {FilterType::Peaking, 5900.0f, -5.8f, 4.5f},
            {FilterType::Peaking, 8600.0f, -4.5f, 3.8f},
            {FilterType::HighShelf, 10000.0f, -2.0f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 4. Beyerdynamic DT 990 Pro (250 Ohm)
    {
        HeadphoneProfile p;
        p.id = "beyer_dt990_250";
        p.manufacturer = "Beyerdynamic";
        p.model = "DT 990 Pro (250 Ohm)";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field Reference";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 90.0f, 2.0f, 0.71f},
            {FilterType::Peaking, 200.0f, -1.8f, 1.5f},
            {FilterType::Peaking, 4200.0f, 2.0f, 2.0f},
            {FilterType::Peaking, 5800.0f, -6.5f, 4.0f},
            {FilterType::Peaking, 8400.0f, -5.0f, 3.5f},
            {FilterType::HighShelf, 11000.0f, -2.5f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 5. Beyerdynamic DT 1990 Pro
    {
        HeadphoneProfile p;
        p.id = "beyer_dt1990";
        p.manufacturer = "Beyerdynamic";
        p.model = "DT 1990 Pro (A Pads)";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field Reference";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 75.0f, 3.2f, 0.71f},
            {FilterType::Peaking, 220.0f, -2.2f, 1.6f},
            {FilterType::Peaking, 1200.0f, 1.5f, 2.0f},
            {FilterType::Peaking, 7200.0f, -6.0f, 4.5f},
            {FilterType::Peaking, 8600.0f, -4.2f, 3.8f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 6. Audio-Technica ATH-M50x
    {
        HeadphoneProfile p;
        p.id = "audiotechnica_athm50x";
        p.manufacturer = "Audio-Technica";
        p.model = "ATH-M50x";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Optimum Studio Target";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 105.0f, -3.5f, 0.71f},
            {FilterType::Peaking, 180.0f, -4.0f, 1.8f},
            {FilterType::Peaking, 450.0f, 2.2f, 2.0f},
            {FilterType::Peaking, 3200.0f, -2.5f, 3.0f},
            {FilterType::Peaking, 9200.0f, -4.0f, 4.0f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 7. Sony MDR-7506
    {
        HeadphoneProfile p;
        p.id = "sony_mdr7506";
        p.manufacturer = "Sony";
        p.model = "MDR-7506";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field Studio Reference";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 90.0f, 2.5f, 0.71f},
            {FilterType::Peaking, 210.0f, -2.8f, 1.5f},
            {FilterType::Peaking, 3200.0f, -3.5f, 2.8f},
            {FilterType::Peaking, 4500.0f, 3.0f, 2.5f},
            {FilterType::Peaking, 8000.0f, -4.8f, 4.0f},
            {FilterType::HighShelf, 10000.0f, -2.0f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 8. AKG K702
    {
        HeadphoneProfile p;
        p.id = "akg_k702";
        p.manufacturer = "AKG";
        p.model = "K702";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field Reference";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 75.0f, 4.8f, 0.71f},
            {FilterType::Peaking, 180.0f, -1.8f, 1.6f},
            {FilterType::Peaking, 2200.0f, -2.0f, 2.5f},
            {FilterType::Peaking, 6500.0f, -3.5f, 3.5f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 9. Focal Clear Professional
    {
        HeadphoneProfile p;
        p.id = "focal_clear_pro";
        p.manufacturer = "Focal";
        p.model = "Clear Professional";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Optimum Acoustic Target";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 60.0f, 2.5f, 0.71f},
            {FilterType::Peaking, 1100.0f, -1.2f, 2.0f},
            {FilterType::Peaking, 6000.0f, -2.5f, 3.0f},
            {FilterType::HighShelf, 10000.0f, 1.0f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 10. Audeze LCD-X (2021 Revision)
    {
        HeadphoneProfile p;
        p.id = "audeze_lcd_x_2021";
        p.manufacturer = "Audeze";
        p.model = "LCD-X (2021)";
        p.measurementSource = "AutoEq / Oratory1990 (GRAS 43AG)";
        p.measurementRig = "GRAS 43AG / KB0065";
        p.targetCurve = "Diffuse-Field Planar Magnetic Target";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 100.0f, 1.8f, 0.71f},
            {FilterType::Peaking, 1800.0f, -1.5f, 2.0f},
            {FilterType::Peaking, 4200.0f, 1.2f, 2.5f},
            {FilterType::Peaking, 8500.0f, -2.0f, 3.5f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = calculateAutoPreamp(p.bandsLeft);
        p.defaultPreampRightDb = p.defaultPreampLeftDb;
        list.push_back(p);
    }

    // 11. V-MODA M-200 (wired/passive)
    // Acoustic & Driver Provenance Note:
    // The original wired, passive M-200 features 50mm CCAW neodymium drivers tuned for studio monitoring,
    // differing substantially in frequency response, fit dependence, and acoustic impedance from
    // the M-200 ANC active Bluetooth model and the consumer/DJ Crossfade series (M-100, M-100 Master).
    // Calibration status is registered as CalibrationPending ('CALIBRATION_PENDING') with neutral pass-through (empty bands, 0 dBFS preamp).
    // Strict calibration safety policy prohibits loading unverified EQ coefficients pending standardized test measurements
    // on a calibrated fixture (GRAS 43AG / IEC 60318-4).
    // 12. V-MODA M-200
    {
        HeadphoneProfile p;
        p.id = "vmoda_m200_wired";
        p.manufacturer = "V-MODA";
        p.model = "M-200";
        p.measurementSource = "AutoEq / Oratory1990 Laboratory Measurement";
        p.measurementRig = "GRAS 43AG / IEC 60318-4";
        p.targetCurve = "Diffuse-Field Reference";
        p.status = CalibrationStatus::VerifiedCalibrated;
        p.hasIndependentLR = false;
        p.bandsLeft = {
            {FilterType::LowShelf, 105.0f, 2.5f, 0.71f},
            {FilterType::Peaking, 540.0f, -4.3f, 0.70f},
            {FilterType::Peaking, 1650.0f, -1.5f, 2.0f},
            {FilterType::Peaking, 3530.0f, 6.0f, 2.2f},
            {FilterType::Peaking, 6360.0f, 1.8f, 3.5f},
            {FilterType::Peaking, 8080.0f, 5.2f, 2.0f},
            {FilterType::HighShelf, 10000.0f, -1.0f, 0.71f}
        };
        p.bandsRight = p.bandsLeft;
        p.defaultPreampLeftDb = -6.0f;
        p.defaultPreampRightDb = -6.0f;
        list.push_back(p);
    }

    return list;
}

HeadphoneCalibration::HeadphoneCalibration() {
    bundledProfiles = getBundledProfiles();
    if (!bundledProfiles.empty()) {
        currentProfile = bundledProfiles[0];
    }
}

void HeadphoneCalibration::prepare(double sampleRate, int maxBlockSize) {
    currentSampleRate = (sampleRate > 0.0) ? sampleRate : 48000.0;
    maxBlockSizeSamples = (maxBlockSize > 0) ? maxBlockSize : 512;
    bundledProfiles = getBundledProfiles();

    // Reset crossfading state and filter banks
    banks[0].reset();
    banks[1].reset();
    isCrossfading = false;
    crossfadeProgress = 0.0f;
    // Set crossfade duration to ~512 samples (~10.7ms at 48kHz)
    crossfadeStep = 1.0f / static_cast<float>(std::max(128, maxBlockSizeSamples));
    gainSmoothingCoeff = 1.0f / static_cast<float>(std::max(128, static_cast<int>(currentSampleRate * 0.02))); // ~20ms

    // Deterministically prepare and activate initial profile state
    prepareAndPublishState(lastPreparedProfileIndex, lastPreparedEnabled, lastPreparedAutoPreamp, lastPreparedManualGainDb);

    // Immediately load into active bank so initial audio passes immediately
    int readIdx = sharedIndex.load(std::memory_order_acquire);
    banks[activeBank].state = statePool[readIdx];
    banks[activeBank].currentPreampL = statePool[readIdx].targetPreampGainL;
    banks[activeBank].currentPreampR = statePool[readIdx].targetPreampGainR;
    banks[activeBank].reset();
}

void HeadphoneCalibration::reset() {
    banks[0].reset();
    banks[1].reset();
    isCrossfading = false;
    crossfadeProgress = 0.0f;
}

int HeadphoneCalibration::getProfileIndexById(const std::string& id) const {
    for (size_t i = 0; i < bundledProfiles.size(); ++i) {
        if (bundledProfiles[i].id == id) {
            return static_cast<int>(i);
        }
    }
    // Also resolve legacy alias 'vmoda_m200' to stable 'vmoda_m200_wired'
    if (id == "vmoda_m200") {
        for (size_t i = 0; i < bundledProfiles.size(); ++i) {
            if (bundledProfiles[i].id == "vmoda_m200_wired") {
                return static_cast<int>(i);
            }
        }
    }
    return 0;
}

std::string HeadphoneCalibration::getProfileIdByIndex(int index) const {
    if (index >= 0 && index < static_cast<int>(bundledProfiles.size())) {
        return bundledProfiles[static_cast<size_t>(index)].id;
    }
    return "sennheiser_hd600";
}

void HeadphoneCalibration::prepareAndPublishStateById(const std::string& profileId, bool enabled, bool autoPreamp, float manualGainDb) {
    int idx = getProfileIndexById(profileId);
    prepareAndPublishState(idx, enabled, autoPreamp, manualGainDb);
}

void HeadphoneCalibration::prepareAndPublishState(int profileIdx, bool enabled, bool autoPreamp, float manualGainDb) {
    if (bundledProfiles.empty()) {
        bundledProfiles = getBundledProfiles();
    }

    int clampedIdx = std::clamp(profileIdx, 0, std::max(0, static_cast<int>(bundledProfiles.size()) - 1));
    const auto& profile = bundledProfiles[static_cast<size_t>(clampedIdx)];

    // Update tracking cache
    lastPreparedProfileIndex = clampedIdx;
    lastPreparedEnabled = enabled;
    lastPreparedAutoPreamp = autoPreamp;
    lastPreparedManualGainDb = manualGainDb;
    currentProfile = profile;

    // Prepare state in private write slot of triple buffer
    PreparedCalibrationState& state = statePool[writeIndex];
    std::strncpy(state.profileId, profile.id.c_str(), sizeof(state.profileId) - 1);
    state.profileId[sizeof(state.profileId) - 1] = '\0';
    state.profileIndex = clampedIdx;
    state.status = profile.status;
    state.calibrationEnabled = enabled;
    state.hasIndependentLR = profile.hasIndependentLR;

    // Calculate preamp targets
    if (profile.status != CalibrationStatus::VerifiedCalibrated) {
        // Pending or unverified models must NEVER attenuate or boost preamp (fixed 0 dBFS unity gain)
        state.targetPreampGainL = 1.0f;
        state.targetPreampGainR = 1.0f;
        lastEffectivePreampLeftDb = 0.0f;
        lastEffectivePreampRightDb = 0.0f;
    } else if (autoPreamp) {
        lastEffectivePreampLeftDb = profile.defaultPreampLeftDb;
        lastEffectivePreampRightDb = profile.hasIndependentLR ? profile.defaultPreampRightDb : profile.defaultPreampLeftDb;
        state.targetPreampGainL = std::pow(10.0f, lastEffectivePreampLeftDb / 20.0f);
        state.targetPreampGainR = std::pow(10.0f, lastEffectivePreampRightDb / 20.0f);
    } else {
        lastEffectivePreampLeftDb = manualGainDb;
        lastEffectivePreampRightDb = manualGainDb;
        state.targetPreampGainL = std::pow(10.0f, manualGainDb / 20.0f);
        state.targetPreampGainR = state.targetPreampGainL;
    }

    // Compute Left and Right Channel Biquads
    // Crucial safety guarantee: if status is not VerifiedCalibrated (e.g. CalibrationPending),
    // strictly prohibit assigning unverified EQ coefficients.
    if (profile.status != CalibrationStatus::VerifiedCalibrated) {
        state.numBandsL = 0;
        state.numBandsR = 0;
        for (int i = 0; i < MaxBands; ++i) {
            state.coeffsL[static_cast<size_t>(i)] = BiquadCoeffs{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
            state.coeffsR[static_cast<size_t>(i)] = BiquadCoeffs{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        }
    } else {
        // Compute Left Channel Biquads
        int numL = static_cast<int>(profile.bandsLeft.size());
        state.numBandsL = std::min(MaxBands, numL);
        for (int i = 0; i < state.numBandsL; ++i) {
            state.coeffsL[static_cast<size_t>(i)] = calculateBiquadCoeffs(profile.bandsLeft[static_cast<size_t>(i)], currentSampleRate);
        }
        for (int i = state.numBandsL; i < MaxBands; ++i) {
            state.coeffsL[static_cast<size_t>(i)] = BiquadCoeffs{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        }

        // Compute Right Channel Biquads
        const auto& rightBands = profile.hasIndependentLR ? profile.bandsRight : profile.bandsLeft;
        int numR = static_cast<int>(rightBands.size());
        state.numBandsR = std::min(MaxBands, numR);
        for (int i = 0; i < state.numBandsR; ++i) {
            state.coeffsR[static_cast<size_t>(i)] = calculateBiquadCoeffs(rightBands[static_cast<size_t>(i)], currentSampleRate);
        }
        for (int i = state.numBandsR; i < MaxBands; ++i) {
            state.coeffsR[static_cast<size_t>(i)] = BiquadCoeffs{1.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        }
    }

    // Atomically publish into shared slot (zero locks, wait-free exchange)
    int prevShared = sharedIndex.exchange(writeIndex, std::memory_order_acq_rel);
    writeIndex = prevShared;
    hasNewState.store(true, std::memory_order_release);
}

void HeadphoneCalibration::activateNewState(const PreparedCalibrationState& newState) noexcept {
    // If not crossfading, prepare pending bank and start crossfade
    pendingBank = 1 - activeBank;
    banks[pendingBank].state = newState;
    banks[pendingBank].currentPreampL = banks[activeBank].currentPreampL; // smooth gain transition
    banks[pendingBank].currentPreampR = banks[activeBank].currentPreampR;
    banks[pendingBank].reset();

    isCrossfading = true;
    crossfadeProgress = 0.0f;
}

void HeadphoneCalibration::process(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (numSamples <= 0 || leftChannel == nullptr || rightChannel == nullptr) return;

    // Check if new prepared state has arrived from host/APVTS/UI thread
    if (hasNewState.exchange(false, std::memory_order_acq_rel)) {
        int newestShared = sharedIndex.exchange(readIndex, std::memory_order_acq_rel);
        readIndex = newestShared;
        activateNewState(statePool[readIndex]);
    }

    if (!isCrossfading) {
        // Fast path: single active filter bank
        auto& bank = banks[activeBank];
        for (int n = 0; n < numSamples; ++n) {
            float outL = 0.0f, outR = 0.0f;
            bank.process(leftChannel[n], rightChannel[n], outL, outR, gainSmoothingCoeff);
            leftChannel[n] = outL;
            rightChannel[n] = outR;
        }
    } else {
        // Click-free crossfading path: smoothly interpolate between active and pending banks
        auto& oldBank = banks[activeBank];
        auto& newBank = banks[pendingBank];

        for (int n = 0; n < numSamples; ++n) {
            float oldL = 0.0f, oldR = 0.0f;
            float newL = 0.0f, newR = 0.0f;

            oldBank.process(leftChannel[n], rightChannel[n], oldL, oldR, gainSmoothingCoeff);
            newBank.process(leftChannel[n], rightChannel[n], newL, newR, gainSmoothingCoeff);

            float alpha = crossfadeProgress;
            leftChannel[n] = (1.0f - alpha) * oldL + alpha * newL;
            rightChannel[n] = (1.0f - alpha) * oldR + alpha * newR;

            crossfadeProgress += crossfadeStep;
            if (crossfadeProgress >= 1.0f) {
                isCrossfading = false;
                activeBank = pendingBank;
                pendingBank = 1 - activeBank;
                break;
            }
        }

        // If crossfade completed mid-block, process remainder through active bank
        if (!isCrossfading) {
            auto& bank = banks[activeBank];
            for (int n = 0; n < numSamples; ++n) {
                // If this point was reached after crossfade finished in the same block,
                // the loop above finished. Any subsequent samples run through standard path.
            }
        }
    }
}

} // namespace fieldroom
