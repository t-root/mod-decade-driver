#pragma once

#include <Arduino.h>

// ESP32-C3 Super Mini pin mapping
// - S1_PIN       -> GPIO1
// - S2_PIN       -> GPIO3
// - L1_PIN       -> GPIO4
// - L2_PIN       -> GPIO5
// - TOUCH_PIN    -> GPIO6 (digital input)
// - LED_R_PIN    -> GPIO7
// - LED_G_PIN    -> GPIO8
// - LED_B_PIN    -> GPIO9
// - DFPlayer TX  -> GPIO20 (ESP32-C3 RX)
// - DFPlayer RX  -> GPIO21 (ESP32-C3 TX)

constexpr uint8_t S1_PIN = 1;
constexpr uint8_t S2_PIN = 3;
constexpr uint8_t L1 = 4;
constexpr uint8_t L2 = 5;
constexpr uint8_t TOUCH_PIN = 6;

constexpr uint8_t LED_R = 7;
constexpr uint8_t LED_G = 8;
constexpr uint8_t LED_B = 9;
