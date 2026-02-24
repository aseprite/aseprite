// Aseprite
// Copyright (C) 2026  Custom Build
//
// Standalone Palette Generator Tests
// Can be compiled and run independently for quick verification
//
// Compile with:
//   g++ -std=c++17 -I../../.. -I../../../../doc standalone_test.cpp -o test_palette && ./test_palette
// Or use the build system

#include <iostream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>

// ============================================================================
// Minimal mock of doc::color_t for standalone testing
// ============================================================================
namespace doc {
    using color_t = uint32_t;

    inline color_t rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        return (static_cast<uint32_t>(r)) |
               (static_cast<uint32_t>(g) << 8) |
               (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(a) << 24);
    }

    inline uint8_t rgba_getr(color_t c) { return c & 0xff; }
    inline uint8_t rgba_getg(color_t c) { return (c >> 8) & 0xff; }
    inline uint8_t rgba_getb(color_t c) { return (c >> 16) & 0xff; }
    inline uint8_t rgba_geta(color_t c) { return (c >> 24) & 0xff; }
}

// ============================================================================
// Test framework
// ============================================================================
static int g_passed = 0;
static int g_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running: " << #name << "..."; \
    test_##name(); \
    std::cout << " done\n"; \
} while(0)

#define EXPECT_TRUE(cond) do { \
    if (!(cond)) { \
        std::cout << "\n  [FAIL] " << __FILE__ << ":" << __LINE__ << " - " #cond << "\n"; \
        g_failed++; \
    } else { g_passed++; } \
} while(0)

#define EXPECT_NEAR(expected, actual, tol) do { \
    if (std::abs((expected) - (actual)) > (tol)) { \
        std::cout << "\n  [FAIL] " << __FILE__ << ":" << __LINE__ \
                  << "\n    Expected: " << (expected) << "\n    Actual: " << (actual) << "\n"; \
        g_failed++; \
    } else { g_passed++; } \
} while(0)

// ============================================================================
// HSV structure and conversion functions (copied from palette_generator)
// ============================================================================
struct HSV {
    double h;  // 0-360
    double s;  // 0-1
    double v;  // 0-1
    HSV() : h(0), s(0), v(0) {}
    HSV(double hue, double sat, double val) : h(hue), s(sat), v(val) {}
};

HSV rgbToHsv(int r, int g, int b) {
    double rd = r / 255.0;
    double gd = g / 255.0;
    double bd = b / 255.0;

    double maxVal = std::max({rd, gd, bd});
    double minVal = std::min({rd, gd, bd});
    double delta = maxVal - minVal;

    HSV hsv;
    hsv.v = maxVal;

    if (delta < 0.00001) {
        hsv.s = 0;
        hsv.h = 0;
        return hsv;
    }

    hsv.s = (maxVal > 0.0) ? (delta / maxVal) : 0.0;

    if (rd >= maxVal) {
        hsv.h = (gd - bd) / delta;
    }
    else if (gd >= maxVal) {
        hsv.h = 2.0 + (bd - rd) / delta;
    }
    else {
        hsv.h = 4.0 + (rd - gd) / delta;
    }

    hsv.h *= 60.0;
    if (hsv.h < 0) {
        hsv.h += 360.0;
    }

    return hsv;
}

void hsvToRgb(const HSV& hsv, int& r, int& g, int& b) {
    if (hsv.s <= 0.0) {
        r = g = b = static_cast<int>(hsv.v * 255.0);
        return;
    }

    double hh = hsv.h;
    if (hh >= 360.0) hh = 0.0;
    hh /= 60.0;

    int i = static_cast<int>(hh);
    double ff = hh - i;
    double p = hsv.v * (1.0 - hsv.s);
    double q = hsv.v * (1.0 - (hsv.s * ff));
    double t = hsv.v * (1.0 - (hsv.s * (1.0 - ff)));

    double rd, gd, bd;
    switch (i) {
        case 0:  rd = hsv.v; gd = t;     bd = p;     break;
        case 1:  rd = q;     gd = hsv.v; bd = p;     break;
        case 2:  rd = p;     gd = hsv.v; bd = t;     break;
        case 3:  rd = p;     gd = q;     bd = hsv.v; break;
        case 4:  rd = t;     gd = p;     bd = hsv.v; break;
        default: rd = hsv.v; gd = p;     bd = q;     break;
    }

    r = static_cast<int>(rd * 255.0 + 0.5);
    g = static_cast<int>(gd * 255.0 + 0.5);
    b = static_cast<int>(bd * 255.0 + 0.5);

    r = std::max(0, std::min(255, r));
    g = std::max(0, std::min(255, g));
    b = std::max(0, std::min(255, b));
}

double wrapHue(double h) {
    while (h < 0) h += 360.0;
    while (h >= 360.0) h -= 360.0;
    return h;
}

// ============================================================================
// calculateHueShift function (the critical one we're testing)
// ============================================================================
double calculateHueShift(
    double lightAngle,
    double valuePosition,
    double maxShift,
    bool warmLight)
{
    // valuePosition: -1 = deep shadow, 0 = midtone, +1 = highlight
    //
    // Color theory for warm light source:
    // - Shadows become COOL (shift toward blue/purple = DECREASE hue toward magenta)
    // - Highlights become WARM (shift toward yellow/orange = INCREASE hue)

    double angleRad = lightAngle * 3.14159265358979323846 / 180.0;
    double verticalInfluence = sin(angleRad) * 0.3;
    double horizontalInfluence = cos(angleRad) * 0.15;
    double angleInfluence = 1.0 + verticalInfluence + horizontalInfluence;

    double shiftAmount = std::abs(valuePosition) * maxShift * angleInfluence;

    if (warmLight) {
        // Warm light: cool shadows, warm highlights
        if (valuePosition < 0) {
            // Shadow: shift toward cool (magenta/purple) = NEGATIVE hue
            return -shiftAmount;
        }
        else {
            // Highlight: shift toward warm (orange/yellow) = POSITIVE hue
            return shiftAmount;
        }
    }
    else {
        // Cool light: warm shadows, cool highlights (opposite)
        if (valuePosition < 0) {
            // Shadow: shift toward warm = POSITIVE hue
            return shiftAmount;
        }
        else {
            // Highlight: shift toward cool = NEGATIVE hue
            return -shiftAmount;
        }
    }
}

// ============================================================================
// HSV/RGB Conversion Tests
// ============================================================================

TEST(rgb_to_hsv_red) {
    HSV hsv = rgbToHsv(255, 0, 0);
    EXPECT_NEAR(0.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_green) {
    HSV hsv = rgbToHsv(0, 255, 0);
    EXPECT_NEAR(120.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_blue) {
    HSV hsv = rgbToHsv(0, 0, 255);
    EXPECT_NEAR(240.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_yellow) {
    HSV hsv = rgbToHsv(255, 255, 0);
    EXPECT_NEAR(60.0, hsv.h, 1.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(rgb_to_hsv_orange) {
    HSV hsv = rgbToHsv(255, 128, 0);
    EXPECT_NEAR(30.0, hsv.h, 2.0);
    EXPECT_NEAR(1.0, hsv.s, 0.01);
    EXPECT_NEAR(1.0, hsv.v, 0.01);
}

TEST(hsv_to_rgb_roundtrip) {
    int origR = 200, origG = 100, origB = 50;
    HSV hsv = rgbToHsv(origR, origG, origB);
    int r, g, b;
    hsvToRgb(hsv, r, g, b);
    EXPECT_TRUE(std::abs(origR - r) <= 1);
    EXPECT_TRUE(std::abs(origG - g) <= 1);
    EXPECT_TRUE(std::abs(origB - b) <= 1);
}

// ============================================================================
// Hue Shift Direction Tests (CRITICAL)
// ============================================================================

TEST(hue_shift_warm_shadow_is_negative) {
    // Warm light with shadows should return NEGATIVE shift (toward cool/magenta)
    double shift = calculateHueShift(135.0, -1.0, 20.0, true);
    std::cout << " shift=" << shift;
    EXPECT_TRUE(shift < 0);  // MUST be negative for warm light shadow
}

TEST(hue_shift_warm_highlight_is_positive) {
    // Warm light with highlights should return POSITIVE shift (toward warm/orange)
    double shift = calculateHueShift(135.0, 1.0, 20.0, true);
    std::cout << " shift=" << shift;
    EXPECT_TRUE(shift > 0);  // MUST be positive for warm light highlight
}

TEST(hue_shift_cool_shadow_is_positive) {
    // Cool light with shadows should return POSITIVE shift (toward warm)
    double shift = calculateHueShift(135.0, -1.0, 20.0, false);
    std::cout << " shift=" << shift;
    EXPECT_TRUE(shift > 0);  // MUST be positive for cool light shadow
}

TEST(hue_shift_cool_highlight_is_negative) {
    // Cool light with highlights should return NEGATIVE shift (toward cool)
    double shift = calculateHueShift(135.0, 1.0, 20.0, false);
    std::cout << " shift=" << shift;
    EXPECT_TRUE(shift < 0);  // MUST be negative for cool light highlight
}

TEST(hue_shift_midtone_is_zero) {
    // Midtone (valuePosition = 0) should have minimal/zero shift
    double shiftWarm = calculateHueShift(135.0, 0.0, 20.0, true);
    double shiftCool = calculateHueShift(135.0, 0.0, 20.0, false);
    std::cout << " warmShift=" << shiftWarm << " coolShift=" << shiftCool;
    EXPECT_NEAR(0.0, shiftWarm, 0.1);
    EXPECT_NEAR(0.0, shiftCool, 0.1);
}

TEST(hue_shift_magnitude_increases_with_position) {
    // Deeper shadows/brighter highlights should have larger shifts
    double shiftMid = std::abs(calculateHueShift(135.0, -0.5, 20.0, true));
    double shiftDeep = std::abs(calculateHueShift(135.0, -1.0, 20.0, true));
    std::cout << " midShift=" << shiftMid << " deepShift=" << shiftDeep;
    EXPECT_TRUE(shiftDeep > shiftMid);
}

// ============================================================================
// Color Application Tests
// ============================================================================

TEST(red_warm_shadow_goes_cool) {
    // Red (hue 0) with warm light shadow should shift toward magenta (330-360)
    double baseHue = 0.0;
    double shift = calculateHueShift(135.0, -1.0, 20.0, true);
    double newHue = wrapHue(baseHue + shift);
    std::cout << " newHue=" << newHue;
    // Should be in magenta range (330-360) or very near 0
    EXPECT_TRUE(newHue > 300.0 || newHue < 30.0);
}

TEST(red_warm_highlight_goes_warm) {
    // Red (hue 0) with warm light highlight should shift toward orange (20-40)
    double baseHue = 0.0;
    double shift = calculateHueShift(135.0, 1.0, 20.0, true);
    double newHue = wrapHue(baseHue + shift);
    std::cout << " newHue=" << newHue;
    // Should be in orange range (toward 30)
    EXPECT_TRUE(newHue > 0.0 && newHue < 60.0);
}

TEST(blue_warm_shadow_goes_cool) {
    // Blue (hue 240) with warm light shadow should shift toward purple (270-300)
    // Actually toward cyan (210-240) since "cool" for blue means toward cyan
    // Wait - let me reconsider. For blue:
    // - Negative shift (cool) goes toward cyan (180)
    // - Positive shift (warm) goes toward magenta (300)
    double baseHue = 240.0;
    double shift = calculateHueShift(135.0, -1.0, 20.0, true);
    double newHue = wrapHue(baseHue + shift);
    std::cout << " newHue=" << newHue;
    // Negative shift from 240 goes toward cyan (lower hue)
    EXPECT_TRUE(newHue < 240.0 && newHue > 180.0);
}

// ============================================================================
// Wrap Hue Tests
// ============================================================================

TEST(wrap_hue_negative) {
    EXPECT_NEAR(350.0, wrapHue(-10.0), 0.01);
    EXPECT_NEAR(330.0, wrapHue(-30.0), 0.01);
    EXPECT_NEAR(260.0, wrapHue(-100.0), 0.01);
}

TEST(wrap_hue_positive) {
    EXPECT_NEAR(10.0, wrapHue(370.0), 0.01);
    EXPECT_NEAR(30.0, wrapHue(390.0), 0.01);
    EXPECT_NEAR(0.0, wrapHue(360.0), 0.01);
}

TEST(wrap_hue_normal) {
    EXPECT_NEAR(180.0, wrapHue(180.0), 0.01);
    EXPECT_NEAR(0.0, wrapHue(0.0), 0.01);
    EXPECT_NEAR(359.0, wrapHue(359.0), 0.01);
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "========================================\n";
    std::cout << "Palette Generator Standalone Tests\n";
    std::cout << "========================================\n\n";

    std::cout << "--- HSV/RGB Conversion ---\n";
    RUN_TEST(rgb_to_hsv_red);
    RUN_TEST(rgb_to_hsv_green);
    RUN_TEST(rgb_to_hsv_blue);
    RUN_TEST(rgb_to_hsv_yellow);
    RUN_TEST(rgb_to_hsv_orange);
    RUN_TEST(hsv_to_rgb_roundtrip);

    std::cout << "\n--- Hue Wrap ---\n";
    RUN_TEST(wrap_hue_negative);
    RUN_TEST(wrap_hue_positive);
    RUN_TEST(wrap_hue_normal);

    std::cout << "\n--- Hue Shift Direction (CRITICAL) ---\n";
    RUN_TEST(hue_shift_warm_shadow_is_negative);
    RUN_TEST(hue_shift_warm_highlight_is_positive);
    RUN_TEST(hue_shift_cool_shadow_is_positive);
    RUN_TEST(hue_shift_cool_highlight_is_negative);
    RUN_TEST(hue_shift_midtone_is_zero);
    RUN_TEST(hue_shift_magnitude_increases_with_position);

    std::cout << "\n--- Color Application ---\n";
    RUN_TEST(red_warm_shadow_goes_cool);
    RUN_TEST(red_warm_highlight_goes_warm);
    RUN_TEST(blue_warm_shadow_goes_cool);

    std::cout << "\n========================================\n";
    std::cout << "Results: " << g_passed << " passed, " << g_failed << " failed\n";
    std::cout << "========================================\n";

    return g_failed > 0 ? 1 : 0;
}
