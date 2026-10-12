#pragma once

#include "common.h"

namespace Localization {

enum eUnitSystem {
	UNITS_SYSTEM = -1,
	UNITS_METRIC,
	UNITS_IMPERIAL
};

extern int32 Units;
bool UsesMetricUnits(void);

inline bool
ResolveMetricUnits(int32 requestedUnits, uint32 systemMeasurement, bool localeAvailable)
{
	if(requestedUnits == UNITS_METRIC)
		return true;
	if(requestedUnits == UNITS_IMPERIAL)
		return false;
	return !localeAvailable || systemMeasurement == 0;
}

}
