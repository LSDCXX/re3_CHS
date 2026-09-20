// Native re3 adaptation of ParticleEx by Fire_Head. See docs/PARTICLEEX.md.
#include "ParticleMgrPS2.h"
#include "FileMgr.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>

namespace REPS2 {
cParticleSystemMgr mod_ParticleSystemManager;
cParticleSystemMgr::cParticleSystemMgr() { memset(this, 0, sizeof(*this)); }
void cParticleSystemMgr::Initialise() {
	LoadParticleData();
	for (int32 i = 0; i < MAX_PARTICLES; ++i) m_aParticles[i].m_pParticles = nil;
}
bool cParticleSystemMgr::LoadParticleData() {
	FILE *file = fopen("ParticleEx/PS2/particle.cfg", "r");
	if (!file) return false;
	cParticleSystemMgr parsed;
	tParticleSystemData *entry = nil;
	const char *expected[] = {"SPARK", "SPARK_SMALL", "WHEEL_DIRT", "WHEEL_WATER", "BLOOD", "BLOOD_SMALL", "BLOOD_SPURT", "DEBRIS", "DEBRIS2", "WATER", "FLAME", "FIREBALL", "GUNFLASH", "GUNFLASH_NOANIM", "GUNSMOKE", "GUNSMOKE2", "SMOKE", "SMOKE_SLOWMOTION", "GARAGEPAINT_SPRAY", "SHARD", "SPLASH", "CARFLAME", "STEAM", "STEAM2", "STEAM_NY", "STEAM_NY_SLOWMOTION", "ENGINE_STEAM", "RAINDROP", "RAINDROP_SMALL", "RAIN_SPLASH", "RAIN_SPLASH_BIGGROW", "RAIN_SPLASHUP", "WATERSPRAY", "EXPLOSION_MEDIUM", "EXPLOSION_LARGE", "EXPLOSION_MFAST", "EXPLOSION_LFAST", "CAR_SPLASH", "BOAT_SPLASH", "BOAT_THRUSTJET", "BOAT_WAKE", "WATER_HYDRANT", "WATER_CANNON", "EXTINGUISH_STEAM", "PED_SPLASH", "PEDFOOT_DUST", "HELI_DUST", "HELI_ATTACK", "ENGINE_SMOKE", "ENGINE_SMOKE2", "CARFLAME_SMOKE", "FIREBALL_SMOKE", "PAINT_SMOKE", "TREE_LEAVES", "CARCOLLISION_DUST", "CAR_DEBRIS", "HELI_DEBRIS", "EXHAUST_FUMES", "RUBBER_SMOKE", "BURNINGRUBBER_SMOKE", "BULLETHIT_SMOKE", "GUNSHELL_FIRST", "GUNSHELL", "GUNSHELL_BUMP1", "GUNSHELL_BUMP2", "TEST", "BIRD_FRONT", "RAINDROP_2D"};
	static_assert(ARRAY_SIZE(expected) == MAX_PARTICLES, "ParticleEx config schema");
	const uint16 frames[] = {1, 1, 5, 5, 1, 1, 1, 1, 1, 5, 1, 1, 4, 4, 1, 5, 5, 5, 5, 4, 1, 1, 5, 5, 5, 5, 5, 4, 1, 5, 5, 2, 3, 6, 6, 6, 6, 4, 5, 5, 1, 4, 5, 5, 4, 5, 5, 1, 1, 1, 1, 1, 5, 1, 1, 4, 4, 5, 5, 1, 5, 1, 1, 1, 1, 1, 4, 4};
	int32 type = 0;
	char line[1024];
	bool valid = true;
	while (fgets(line, sizeof(line), file)) {
		if (!strchr(line, '\n') && !feof(file)) { valid = false; break; }
		char *value = strtok(line, " \t\r\n");
		if (!value || *value == ';') continue;
		if (type >= MAX_PARTICLES || strcmp(value, expected[type])) { valid = false; break; }
		int32 param = CFG_PARAM_FIRST;
		do {
			if (param >= MAX_CFG_PARAMS) { valid = false; break; }
			if (param != CFG_PARAM_PARTICLE_TYPE_NAME) {
				char *end = nil;
				const double number = strtod(value, &end);
				if (end == value || *end || !std::isfinite(number) || fabs(number) > 10000000.0) { valid = false; break; }
				if ((param == CFG_PARAM_START_ANIMATION_FRAME || param == CFG_PARAM_FINAL_ANIMATION_FRAME) &&
				    (number < 0 || number > 65534 || floor(number) != number)) { valid = false; break; }
			}
			switch (param) {
					case CFG_PARAM_PARTICLE_TYPE_NAME:
						assert(type < MAX_PARTICLES);
						entry = &parsed.m_aParticles[type];
						assert(entry != NULL);
						entry->m_Type = (tParticleType)type++;
						strncpy(entry->m_aName, value, sizeof(entry->m_aName) - 1);
						break;

					case CFG_PARAM_RENDER_COLOURING_R:
						entry->m_RenderColouring.red = atoi(value);
						break;

					case CFG_PARAM_RENDER_COLOURING_G:
						entry->m_RenderColouring.green = atoi(value);
						break;

					case CFG_PARAM_RENDER_COLOURING_B:
						entry->m_RenderColouring.blue = atoi(value);
						break;

					case CFG_PARAM_INITIAL_COLOR_VARIATION:
						entry->m_InitialColorVariation = Clamp(atoi(value), 0, 100);
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_R:
						entry->m_FadeDestinationColor.red = atoi(value);
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_G:
						entry->m_FadeDestinationColor.green = atoi(value);
						break;

					case CFG_PARAM_FADE_DESTINATION_COLOR_B:
						entry->m_FadeDestinationColor.blue = atoi(value);
						break;

					case CFG_PARAM_COLOR_FADE_TIME:
						entry->m_ColorFadeTime = atoi(value);
						break;

					case CFG_PARAM_DEFAULT_INITIAL_RADIUS:
						entry->m_fDefaultInitialRadius = atof(value);
						break;

					case CFG_PARAM_EXPANSION_RATE:
						entry->m_fExpansionRate = atof(value);
						break;

					case CFG_PARAM_INITIAL_INTENSITY:
						entry->m_nFadeToBlackInitialIntensity = atoi(value);
						break;

					case CFG_PARAM_FADE_TIME:
						entry->m_nFadeToBlackTime = atoi(value);
						break;

					case CFG_PARAM_FADE_AMOUNT:
						entry->m_nFadeToBlackAmount = atoi(value);
						break;

					case CFG_PARAM_INITIAL_ALPHA_INTENSITY:
						entry->m_nFadeAlphaInitialIntensity = atoi(value);
						break;

					case CFG_PARAM_FADE_ALPHA_TIME:
						entry->m_nFadeAlphaTime = atoi(value);
						break;

					case CFG_PARAM_FADE_ALPHA_AMOUNT:
						entry->m_nFadeAlphaAmount = atoi(value);
						break;

					case CFG_PARAM_INITIAL_ANGLE:
						entry->m_nZRotationInitialAngle = atoi(value);
						break;

					case CFG_PARAM_CHANGE_TIME:
						entry->m_nZRotationChangeTime = atoi(value);
						break;

					case CFG_PARAM_ANGLE_CHANGE_AMOUNT:
						entry->m_nZRotationAngleChangeAmount = atoi(value);
						break;

					case CFG_PARAM_INITIAL_Z_RADIUS:
						entry->m_fInitialZRadius = atof(value);
						break;

					case CFG_PARAM_Z_RADIUS_CHANGE_TIME:
						entry->m_nZRadiusChangeTime = atoi(value);
						break;

					case CFG_PARAM_Z_RADIUS_CHANGE_AMOUNT:
						entry->m_fZRadiusChangeAmount = atof(value);
						break;

					case CFG_PARAM_ANIMATION_SPEED:
						entry->m_nAnimationSpeed = atoi(value);
						break;

					case CFG_PARAM_START_ANIMATION_FRAME:
						entry->m_nStartAnimationFrame = atoi(value);
						break;

					case CFG_PARAM_FINAL_ANIMATION_FRAME:
						entry->m_nFinalAnimationFrame = atoi(value);
						break;

					case CFG_PARAM_ROTATION_SPEED:
						entry->m_nRotationSpeed = atoi(value);
						break;

					case CFG_PARAM_GRAVITATIONAL_ACCELERATION:
						entry->m_fGravitationalAcceleration = atof(value);
						break;

					case CFG_PARAM_FRICTION_DECCELERATION:
						entry->m_nFrictionDecceleration = atoi(value);
						break;

					case CFG_PARAM_LIFE_SPAN:
						entry->m_nLifeSpan = atoi(value);
						break;

					case CFG_PARAM_POSITION_RANDOM_ERROR:
						entry->m_fPositionRandomError = atof(value);
						break;

					case CFG_PARAM_VELOCITY_RANDOM_ERROR:
						entry->m_fVelocityRandomError = atof(value);
						break;

					case CFG_PARAM_EXPANSION_RATE_ERROR:
						entry->m_fExpansionRateError = atof(value);
						break;

					case CFG_PARAM_ROTATION_RATE_ERROR:
						entry->m_nRotationRateError = atoi(value);
						break;

					case CFG_PARAM_LIFE_SPAN_ERROR_SHAPE:
						entry->m_nLifeSpanErrorShape = atoi(value);
						break;

					case CFG_PARAM_TRAIL_LENGTH_MULTIPLIER:
						entry->m_fTrailLengthMultiplier = atof(value);
						break;

					case CFG_PARAM_PARTICLE_CREATE_RANGE:
						entry->m_fCreateRange = SQR(atof(value));
						break;

					case CFG_PARAM_FLAGS:
						entry->Flags = atoi(value);
						break;
			}
			++param;
			value = strtok(nil, " \t\r\n");
		} while (value);
		if (!valid || param != MAX_CFG_PARAMS) { valid = false; break; }
	}
	fclose(file);
	if (!valid || type != MAX_PARTICLES) return false;
	// Retain texture bindings when a valid config is reloaded.
	for (int32 i = 0; i < MAX_PARTICLES; ++i) {
		parsed.m_aParticles[i].m_ppRaster = m_aParticles[i].m_ppRaster;
		parsed.m_aParticles[i].m_nRasterCount = frames[i];
	}
	*this = parsed;
	return true;
}
}
