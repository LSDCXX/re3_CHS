#pragma once

// XInput-compatible trigger range. Retain the full usable travel after the
// resting dead zone; digital keyboard inputs (255) still produce full power.
inline short ScaleTriggerPressure(short pressure)
{
	const int deadZone = 30;
	if (pressure <= deadZone) return 0;
	if (pressure >= 255) return 255;
	return (pressure - deadZone) * 255 / (255 - deadZone);
}
