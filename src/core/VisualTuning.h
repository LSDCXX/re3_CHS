#pragma once

// Native distance preset based on the requested MixSets-style configuration.
// These are base world distances; existing camera multipliers still apply.
namespace VisualTuning {
const float VehicleHighDetail = 200.0f;
const float VehicleLowDetail = 250.0f;
const float VehicleFade = 260.0f;
const float VehicleDespawnOnScreen = 180.0f;
const float VehicleDespawnOffScreen = 90.0f;
const float VehicleShadow = 300.0f;
const float PedShadow = 300.0f;
const float TrafficLight = 300.0f;
}
