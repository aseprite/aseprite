// Aseprite
// Copyright (C) 2026  Custom Build
//
// Auto-Shade Tool - Test Framework Header

#pragma once

#include <cstdlib>
#include <iostream>
#include <cmath>
#include <string>

// Test result tracking
static int g_testsPassed = 0;
static int g_testsFailed = 0;

template<typename T, typename U>
inline void expect_eq(const T& expected, const U& value,
                      const char* testName, const char* file, const int line) {
    if (expected != value) {
        std::cout << "[FAIL] " << testName << "\n"
                  << "  " << file << ":" << line << "\n"
                  << "  Expected: " << expected << "\n"
                  << "  Actual: " << value << std::endl;
        g_testsFailed++;
    }
    else {
        g_testsPassed++;
    }
}

inline void expect_near(double expected, double value, double tolerance,
                        const char* testName, const char* file, const int line) {
    if (std::abs(expected - value) > tolerance) {
        std::cout << "[FAIL] " << testName << "\n"
                  << "  " << file << ":" << line << "\n"
                  << "  Expected: " << expected << " (+/- " << tolerance << ")\n"
                  << "  Actual: " << value << std::endl;
        g_testsFailed++;
    }
    else {
        g_testsPassed++;
    }
}

inline void expect_true(bool value, const char* testName, const char* file, const int line) {
    if (!value) {
        std::cout << "[FAIL] " << testName << "\n"
                  << "  " << file << ":" << line << "\n"
                  << "  Expected: true\n"
                  << "  Actual: false" << std::endl;
        g_testsFailed++;
    }
    else {
        g_testsPassed++;
    }
}

inline void expect_false(bool value, const char* testName, const char* file, const int line) {
    if (value) {
        std::cout << "[FAIL] " << testName << "\n"
                  << "  " << file << ":" << line << "\n"
                  << "  Expected: false\n"
                  << "  Actual: true" << std::endl;
        g_testsFailed++;
    }
    else {
        g_testsPassed++;
    }
}

// Hue comparison that handles wrap-around (e.g., 359 is close to 1)
inline void expect_hue_near(double expected, double actual, double tolerance,
                            const char* testName, const char* file, const int line) {
    double diff = std::abs(expected - actual);
    // Handle wrap-around: 359 and 1 are only 2 apart, not 358
    if (diff > 180.0) {
        diff = 360.0 - diff;
    }
    if (diff > tolerance) {
        std::cout << "[FAIL] " << testName << "\n"
                  << "  " << file << ":" << line << "\n"
                  << "  Expected hue: " << expected << " (+/- " << tolerance << ")\n"
                  << "  Actual hue: " << actual << "\n"
                  << "  Difference: " << diff << std::endl;
        g_testsFailed++;
    }
    else {
        g_testsPassed++;
    }
}

inline void print_test_summary() {
    std::cout << "\n========================================\n";
    std::cout << "Test Summary: " << g_testsPassed << " passed, "
              << g_testsFailed << " failed\n";
    std::cout << "========================================\n";
}

#define EXPECT_EQ(expected, value) \
    expect_eq(expected, value, __FUNCTION__, __FILE__, __LINE__)

#define EXPECT_NEAR(expected, value, tolerance) \
    expect_near(expected, value, tolerance, __FUNCTION__, __FILE__, __LINE__)

#define EXPECT_TRUE(value) \
    expect_true(value, __FUNCTION__, __FILE__, __LINE__)

#define EXPECT_FALSE(value) \
    expect_false(value, __FUNCTION__, __FILE__, __LINE__)

#define EXPECT_HUE_NEAR(expected, value, tolerance) \
    expect_hue_near(expected, value, tolerance, __FUNCTION__, __FILE__, __LINE__)

#define TEST(name) void test_##name()

#define RUN_TEST(name) \
    do { \
        std::cout << "Running: " << #name << "..." << std::endl; \
        test_##name(); \
    } while(0)
