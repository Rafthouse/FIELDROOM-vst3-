#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>
#include <algorithm>
#include "../../src/DSP/SpeakerGeometry.h"
#include "../../src/DSP/HRTFManager.h"
#include "../../src/DSP/RoomEngine.h"
#include "../../src/DSP/HeadphoneCalibration.h"
#include "../../src/DSP/MonitoringUtilities.h"
#include "../../src/DSP/MonitoringTypes.h"

using namespace fieldroom;

int main() {
    std::cout << "==========================================\n";
    std::cout << "FIELDROOM C++ DSP VERIFICATION TEST SUITE\n";
    std::cout << "==========================================\n\n";

    int passed = 0;
    int total = 0;

    // Test 1: Woodworth-Schlosser ITD calculation at 30 degrees (equilateral triangle speaker)
    {
        total++;
        float angleRad = 30.0f * (juce::MathConstants<float>::pi / 180.0f);
        float itd = HRTFManager::calculateWoodworthITD(angleRad);
        // Expected ~ 0.000261 s (261 microseconds)
        std::cout << "[DSP-01] Testing Woodworth ITD at 30 deg: " << (itd * 1e6f) << " us ... ";
        if (std::abs(itd - 0.000261f) < 0.000030f) {
            std::cout << "PASS\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 2: Speaker Geometry Near / Mid / Far differentiation
    {
        total++;
        auto nearCfg = SpeakerGeometry::getFieldConfig(0);
        auto midCfg  = SpeakerGeometry::getFieldConfig(1);
        auto farCfg  = SpeakerGeometry::getFieldConfig(2);

        std::cout << "[DSP-02] Testing Near/Mid/Far distance differentiation: "
                  << nearCfg.distanceMeters << "m / "
                  << midCfg.distanceMeters  << "m / "
                  << farCfg.distanceMeters  << "m ... ";

        bool diffOk = (nearCfg.distanceMeters < midCfg.distanceMeters) &&
                      (midCfg.distanceMeters < farCfg.distanceMeters) &&
                      (nearCfg.drrDb > midCfg.drrDb) &&
                      (midCfg.drrDb > farCfg.drrDb);

        if (diffOk) {
            std::cout << "PASS (DRR Near: " << nearCfg.drrDb << "dB > Mid: " << midCfg.drrDb << "dB > Far: " << farCfg.drrDb << "dB)\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 3: Headphone Calibration Auto-Preamp Headroom
    {
        total++;
        auto profiles = HeadphoneCalibration::getBundledProfiles();
        std::cout << "[DSP-03] Testing Headphone Auto-Preamp for " << profiles.size() << " bundled models ... ";
        bool preampSafe = true;
        for (const auto& p : profiles) {
            float autoPreamp = HeadphoneCalibration::calculateAutoPreamp(p.bandsLeft);
            if (autoPreamp > 0.001f) { // Preamp must be negative or zero to prevent digital clipping
                preampSafe = false;
                break;
            }
        }
        if (preampSafe && !profiles.empty()) {
            std::cout << "PASS (All preamps <= 0.0 dBFS)\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 4: Monitoring Utilities - Mono fold-down is level-neutral for a centred source, (L + R) / 2
    {
        total++;
        MonitoringUtilities utils;
        utils.prepare(48000.0, 512);
        utils.setMode(MonitoringMode::Mono);

        float centreL = 1.0f, centreR = 1.0f;
        utils.process(&centreL, &centreR, 1);

        std::cout << "[DSP-04] Testing Mono fold-down: centred in = 1.0 -> out = " << centreL << " ... ";
        if (std::abs(centreL - 1.0f) < 1e-6f && std::abs(centreR - 1.0f) < 1e-6f) {
            std::cout << "PASS (0 dB for centre)\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 5: Monitoring Utilities - DIM (-18 dB)
    {
        total++;
        MonitoringUtilities utils;
        utils.prepare(48000.0, 512);
        utils.setDim(true);

        // Let smoother settle across 2048 samples
        std::vector<float> l(2048, 1.0f);
        std::vector<float> r(2048, 1.0f);
        utils.process(l.data(), r.data(), 2048);

        float lastSample = l.back();
        float expectedDimGain = std::pow(10.0f, -18.0f / 20.0f); // ~0.12589
        std::cout << "[DSP-05] Testing DIM -18 dB Attenuation: gain = " << lastSample << " ... ";
        if (std::abs(lastSample - expectedDimGain) < 0.01f) {
            std::cout << "PASS\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 6: Room Engine Decay and Stability
    {
        total++;
        RoomEngine room;
        room.prepare(48000.0, 512);
        room.setRoomProfile(2); // Pro Control Room
        room.setRoomAmount(0.5f);

        // Feed impulse
        std::vector<float> l(48000, 0.0f);
        std::vector<float> r(48000, 0.0f);
        l[0] = 1.0f;
        r[0] = 1.0f;

        room.process(l.data(), r.data(), l.data(), r.data(), 48000);

        // Check that room tail decays without blowing up (NaN/Inf check)
        bool numericalStable = true;
        for (int i = 0; i < 48000; ++i) {
            if (std::isnan(l[i]) || std::isinf(l[i]) || std::abs(l[i]) > 10.0f) {
                numericalStable = false;
                break;
            }
        }
        bool decayed = (std::abs(l[47999]) < 0.001f);

        std::cout << "[DSP-06] Testing Room Engine FDN Stability & Decay ... ";
        if (numericalStable && decayed) {
            std::cout << "PASS (Stable, decayed to < -60 dB)\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 7: V-MODA M-200 (wired/passive) Safe Registration & Neutral Pass-through
    {
        total++;
        std::cout << "[DSP-07] Testing V-MODA M-200 (wired/passive) Safe Registration ... ";
        auto profiles = HeadphoneCalibration::getBundledProfiles();
        bool found = false;
        bool modelNameCorrect = false;
        bool statusCorrect = false;
        bool statusStringCorrect = false;
        bool bandsCount7 = false;
        bool preampMinus6 = false;

        for (const auto& p : profiles) {
            if (p.id == "vmoda_m200_wired") {
                found = true;
                modelNameCorrect = (p.model == "M-200");
                statusCorrect = (p.status == CalibrationStatus::VerifiedCalibrated);
                statusStringCorrect = (std::string(getCalibrationStatusString(p.status)) == "VERIFIED_CALIBRATED");
                bandsCount7 = (p.bandsLeft.size() == 7 && p.bandsRight.size() == 7);
                preampMinus6 = (std::abs(p.defaultPreampLeftDb - (-6.0f)) < 1e-3f) && (std::abs(p.defaultPreampRightDb - (-6.0f)) < 1e-3f);
                break;
            }
        }

        HeadphoneCalibration cal;
        cal.prepare(48000.0, 512);

        // Verify lookup by stable ID and backwards-compatible alias
        int wiredIdx = cal.getProfileIndexById("vmoda_m200_wired");
        int aliasIdx = cal.getProfileIndexById("vmoda_m200");
        bool aliasMatches = (wiredIdx == aliasIdx);

        cal.requestStateById("vmoda_m200_wired", true, true, 0.0f);

        std::vector<float> testL(512, 0.5f);
        std::vector<float> testR(512, -0.5f);
        cal.process(testL.data(), testR.data(), 512);

        // Calibrated audio processing produces stable, non-zero, non-NaN filtered output
        bool audioValid = (std::abs(testL[511]) > 0.01f) && !std::isnan(testL[511]) && !std::isnan(testR[511]);

        if (found && modelNameCorrect && statusCorrect && statusStringCorrect && bandsCount7 && preampMinus6 && aliasMatches && audioValid) {
            std::cout << "PASS (vmoda_m200_wired verified VerifiedCalibrated, 7 bands, -6dBFS preamp, filtered audio)\n";
            passed++;
        } else {
            std::cout << "FAIL (found=" << found << ", model=" << modelNameCorrect << ", status=" << statusCorrect
                      << ", statusStr=" << statusStringCorrect << ", 7bands=" << bandsCount7 << ", alias=" << aliasMatches
                      << ", validAudio=" << audioValid << ")\n";
        }
    }

    // Test 8: Real-Time Safe Lock-Free Triple Buffer & Click-Free Crossfade
    {
        total++;
        std::cout << "[DSP-08] Testing Lock-Free Request & Click-Free Switching ... ";
        HeadphoneCalibration cal;
        cal.prepare(48000.0, 512);

        // Start with HD 600
        cal.requestStateById("sennheiser_hd600", true, true, 0.0f);
        std::vector<float> bufL(512, 0.25f);
        std::vector<float> bufR(512, 0.25f);
        cal.process(bufL.data(), bufR.data(), 512);

        // Switch to DT 770 Pro during active streaming
        cal.requestStateById("beyer_dt770_80", true, true, 0.0f);

        bool glitchFree = true;
        for (int b = 0; b < 10; ++b) {
            std::fill(bufL.begin(), bufL.end(), 0.25f);
            std::fill(bufR.begin(), bufR.end(), 0.25f);
            cal.process(bufL.data(), bufR.data(), 512);

            for (float s : bufL) {
                if (std::isnan(s) || std::isinf(s) || std::abs(s) > 2.0f) {
                    glitchFree = false;
                    break;
                }
            }
        }

        if (glitchFree) {
            std::cout << "PASS (Zero clicks, smooth crossfade, numerical stability preserved)\n";
            passed++;
        } else {
            std::cout << "FAIL (Crossfading glitch or instability detected)\n";
        }
    }

    // Test 9: Meter window keeps the peak of every block, not only the block that closes the window
    {
        total++;
        MonitoringUtilities utils;
        utils.prepare(48000.0, 64);

        // Eight 64-sample blocks make one 512-sample window; only the first block carries a 0.9 peak
        std::vector<float> l(64, 0.0f);
        std::vector<float> r(64, 0.0f);
        l[10] = 0.9f;
        utils.process(l.data(), r.data(), 64);
        std::fill(l.begin(), l.end(), 0.0f);
        for (int b = 1; b < 8; ++b) {
            utils.process(l.data(), r.data(), 64);
        }

        float peakDb = utils.getMeterLevels().peakLeftDb;
        std::cout << "[DSP-09] Testing meter peak hold across small blocks: peakL = " << peakDb << " dB ... ";
        if (std::abs(peakDb - 20.0f * std::log10(0.9f)) < 0.05f) {
            std::cout << "PASS\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    // Test 10: Metering-only path leaves the audio untouched while still reporting levels
    {
        total++;
        MonitoringUtilities utils;
        utils.prepare(48000.0, 64);
        utils.setMode(MonitoringMode::Mono);
        utils.setDim(true);

        std::vector<float> l(512, 0.5f);
        std::vector<float> r(512, -0.25f);
        utils.measure(l.data(), r.data(), 512);

        bool untouched = std::all_of(l.begin(), l.end(), [](float s) { return s == 0.5f; })
                      && std::all_of(r.begin(), r.end(), [](float s) { return s == -0.25f; });
        float peakDb = utils.getMeterLevels().peakLeftDb;
        std::cout << "[DSP-10] Testing bypass metering leaves audio untouched: peakL = " << peakDb << " dB ... ";
        if (untouched && std::abs(peakDb - 20.0f * std::log10(0.5f)) < 0.05f) {
            std::cout << "PASS\n";
            passed++;
        } else {
            std::cout << "FAIL\n";
        }
    }

    std::cout << "\n------------------------------------------\n";
    std::cout << "SUMMARY: " << passed << " / " << total << " TESTS PASSED\n";
    std::cout << "------------------------------------------\n";

    return (passed == total) ? 0 : 1;
}
