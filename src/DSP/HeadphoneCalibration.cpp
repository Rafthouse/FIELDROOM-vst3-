#include "HeadphoneCalibration.h"
#include <algorithm>
#include <cstdio>
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

    // 11. V-MODA M-200
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

HeadphoneCalibration::HeadphoneCalibration()
    : bundledProfiles(getBundledProfiles()) {
    requested.store(packRequest(Request{}), std::memory_order_relaxed);
}

uint64_t HeadphoneCalibration::packRequest(const Request& r) noexcept {
    const int index = std::clamp(r.profileIndex, 0, 255);
    const int gainCentiDb = static_cast<int>(std::lround(std::clamp(r.manualGainDb, -60.0f, 60.0f) * 100.0f));
    uint64_t packed = static_cast<uint64_t>(index);
    packed |= static_cast<uint64_t>(r.enabled ? 1 : 0) << 8;
    packed |= static_cast<uint64_t>(r.autoPreamp ? 1 : 0) << 9;
    packed |= static_cast<uint64_t>(static_cast<uint16_t>(static_cast<int16_t>(gainCentiDb))) << 16;
    return packed;
}

HeadphoneCalibration::Request HeadphoneCalibration::unpackRequest(uint64_t packed) noexcept {
    Request r;
    r.profileIndex = static_cast<int>(packed & 0xFFu);
    r.enabled = ((packed >> 8) & 1u) != 0;
    r.autoPreamp = ((packed >> 9) & 1u) != 0;
    const auto gainRaw = static_cast<int16_t>(static_cast<uint16_t>((packed >> 16) & 0xFFFFu));
    r.manualGainDb = static_cast<float>(gainRaw) * 0.01f;
    return r;
}

HeadphoneCalibration::PreparedCalibrationState HeadphoneCalibration::makeTableEntry(int profileIndex) const {
    PreparedCalibrationState s;
    const auto& profile = bundledProfiles[static_cast<size_t>(profileIndex)];
    s.profileIndex = profileIndex;
    s.status = profile.status;
    s.hasIndependentLR = profile.hasIndependentLR;

    if (profile.status != CalibrationStatus::VerifiedCalibrated) {
        // Pending or unverified models never attenuate or boost and are never filtered
        return s;
    }

    s.autoPreampGainL = std::pow(10.0f, profile.defaultPreampLeftDb / 20.0f);
    s.autoPreampGainR = std::pow(10.0f, (profile.hasIndependentLR ? profile.defaultPreampRightDb : profile.defaultPreampLeftDb) / 20.0f);

    const auto& rightBands = profile.hasIndependentLR ? profile.bandsRight : profile.bandsLeft;
    s.numBandsL = static_cast<int>(std::min<size_t>(MaxBands, profile.bandsLeft.size()));
    s.numBandsR = static_cast<int>(std::min<size_t>(MaxBands, rightBands.size()));
    for (int i = 0; i < s.numBandsL; ++i) {
        s.coeffsL[static_cast<size_t>(i)] = calculateBiquadCoeffs(profile.bandsLeft[static_cast<size_t>(i)], currentSampleRate);
    }
    for (int i = 0; i < s.numBandsR; ++i) {
        s.coeffsR[static_cast<size_t>(i)] = calculateBiquadCoeffs(rightBands[static_cast<size_t>(i)], currentSampleRate);
    }
    return s;
}

void HeadphoneCalibration::prepare(double sampleRate, int maxBlockSize) {
    currentSampleRate = (sampleRate > 0.0) ? sampleRate : 48000.0;
    maxBlockSizeSamples = (maxBlockSize > 0) ? maxBlockSize : 512;
    gainSmoothingCoeff = 1.0f / static_cast<float>(std::max(128.0, currentSampleRate * 0.02)); // about 20 ms

    // Everything that allocates or computes filter coefficients happens here, never on the audio thread
    bundledProfiles = getBundledProfiles();
    table.clear();
    table.reserve(bundledProfiles.size());
    for (int i = 0; i < static_cast<int>(bundledProfiles.size()); ++i) {
        table.push_back(makeTableEntry(i));
    }

    reset();
    const uint64_t current = requested.load(std::memory_order_acquire);
    buildBank(banks[activeBank], unpackRequest(current), nullptr);
    applied = current; // the initial bank already matches the request, so no crossfade at start
}

void HeadphoneCalibration::reset() noexcept {
    banks[0].reset();
    banks[1].reset();
    isCrossfading = false;
    crossfadeProgress = 0.0f;
}

void HeadphoneCalibration::requestState(int profileIndex, bool enabled, bool autoPreamp, float manualGainDb) noexcept {
    requested.store(packRequest(Request{profileIndex, enabled, autoPreamp, manualGainDb}), std::memory_order_release);
}

void HeadphoneCalibration::requestStateById(const std::string& profileId, bool enabled, bool autoPreamp, float manualGainDb) {
    requestState(getProfileIndexById(profileId), enabled, autoPreamp, manualGainDb);
}

void HeadphoneCalibration::buildBank(FilterBank& bank, const Request& req, const FilterBank* crossfadeFrom) noexcept {
    if (table.empty()) return;

    const int index = std::clamp(req.profileIndex, 0, static_cast<int>(table.size()) - 1);
    const PreparedCalibrationState& base = table[static_cast<size_t>(index)];

    // POD copy of the precomputed coefficients, then the request-dependent fields
    bank.state = base;
    bank.state.calibrationEnabled = req.enabled;

    if (base.status != CalibrationStatus::VerifiedCalibrated) {
        bank.state.targetPreampGainL = 1.0f;
        bank.state.targetPreampGainR = 1.0f;
    } else if (req.autoPreamp) {
        bank.state.targetPreampGainL = base.autoPreampGainL;
        bank.state.targetPreampGainR = base.autoPreampGainR;
    } else {
        const float manual = std::pow(10.0f, req.manualGainDb / 20.0f);
        bank.state.targetPreampGainL = manual;
        bank.state.targetPreampGainR = manual;
    }

    bank.reset();
    if (crossfadeFrom != nullptr) {
        bank.currentPreampL = crossfadeFrom->currentPreampL; // smooth from where the old bank is
        bank.currentPreampR = crossfadeFrom->currentPreampR;
    } else {
        bank.currentPreampL = bank.state.targetPreampGainL; // snap on initial load
        bank.currentPreampR = bank.state.targetPreampGainR;
    }
}

void HeadphoneCalibration::process(float* leftChannel, float* rightChannel, int numSamples) noexcept {
    if (numSamples <= 0 || leftChannel == nullptr || rightChannel == nullptr) return;

    // A new request is only taken up while no fade is running, so a fade is never cut short
    if (!isCrossfading) {
        const uint64_t current = requested.load(std::memory_order_acquire);
        if (current != applied) {
            applied = current;
            pendingBank = 1 - activeBank;
            buildBank(banks[pendingBank], unpackRequest(current), &banks[activeBank]);
            isCrossfading = true;
            crossfadeProgress = 0.0f;
        }
    }

    for (int n = 0; n < numSamples; ++n) {
        if (isCrossfading) {
            float oldL = 0.0f, oldR = 0.0f;
            float newL = 0.0f, newR = 0.0f;
            banks[activeBank].process(leftChannel[n], rightChannel[n], oldL, oldR, gainSmoothingCoeff);
            banks[pendingBank].process(leftChannel[n], rightChannel[n], newL, newR, gainSmoothingCoeff);

            const float alpha = crossfadeProgress;
            leftChannel[n] = (1.0f - alpha) * oldL + alpha * newL;
            rightChannel[n] = (1.0f - alpha) * oldR + alpha * newR;

            crossfadeProgress += crossfadeStep;
            if (crossfadeProgress >= 1.0f) {
                isCrossfading = false;
                activeBank = pendingBank;
                crossfadeProgress = 0.0f;
            }
        } else {
            float outL = 0.0f, outR = 0.0f;
            banks[activeBank].process(leftChannel[n], rightChannel[n], outL, outR, gainSmoothingCoeff);
            leftChannel[n] = outL;
            rightChannel[n] = outR;
        }
    }
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

} // namespace fieldroom
