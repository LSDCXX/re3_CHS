#pragma once

// Native distance preset based on the requested MixSets-style configuration.
// These are base world distances; existing camera multipliers still apply.
namespace VisualTuning {
constexpr float VehicleHighDetail = 200.0f;
constexpr float VehicleLowDetail = 250.0f;
constexpr float VehicleFade = 260.0f;
constexpr float VehicleDespawnOnScreen = 180.0f;
constexpr float VehicleDespawnOffScreen = 90.0f;
constexpr float VehicleShadow = 300.0f;
constexpr float PedShadow = 300.0f;
constexpr float TrafficLight = 300.0f;
}
