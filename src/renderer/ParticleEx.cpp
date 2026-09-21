#include "common.h"
#include "ParticleEx.h"
#include "General.h"
#include "Vehicle.h"
#include "ParticleObject.h"

int32 ParticleEx::SelectedSystem = ParticleEx::PC;
ParticleEx::System ParticleEx::ActiveSystem = ParticleEx::PC;

void ParticleEx::Initialise()
{
	ActiveSystem = PC;
	if (SelectedSystem == PS2Xbox) {
		if (REPS2::ParticleEngine::LoadResources()) {
			REPS2::ParticleEngine::Initialise();
			if (REXBOX::ParticleEngine::LoadResources()) {
				REXBOX::ParticleEngine::Initialise();
				ActiveSystem = PS2Xbox;
			} else {
				REPS2::ParticleEngine::Shutdown();
			}
		}
	} else if (SelectedSystem == PS2 && REPS2::ParticleEngine::LoadResources()) {
		REPS2::ParticleEngine::Initialise();
		ActiveSystem = PS2;
	} else if (SelectedSystem == Xbox && REXBOX::ParticleEngine::LoadResources()) {
		REXBOX::ParticleEngine::Initialise();
		ActiveSystem = Xbox;
	}
	if (SelectedSystem != PC && ActiveSystem == PC)
		debug("ParticleEx: invalid selection or missing/invalid resources; using PC particles.\n");
}

void ParticleEx::Shutdown()
{
	if (UsesPS2Emitters()) REPS2::ParticleEngine::Shutdown();
	if (UsesXboxFire()) REXBOX::ParticleEngine::Shutdown();
	ActiveSystem = PC;
}

REXBOX::tParticleType ParticleEx::XboxType(::tParticleType type)
{
	// Xbox inserts CARFLAME_MOVING after CARFLAME. Preserve all following IDs.
	return static_cast<REXBOX::tParticleType>(int(type) + (type > PARTICLE_CARFLAME ? 1 : 0));
}

void ParticleEx::AddVehicleFire(CVector position, const CVector &speed)
{
	position.x += CGeneral::GetRandomNumberInRange(-0.25f, 0.25f);
	position.y += CGeneral::GetRandomNumberInRange(-0.25f, 0.25f);
	CVector velocity(0.0f, 0.0f, CGeneral::GetRandomNumberInRange(0.0035f, 0.0735f) * 0.35f);
	const bool moving = (SQR(speed.x) + SQR(speed.y)) > 0.003f;
	if (moving) position.z += 0.2f;
	REXBOX::ParticleEngine::AddParticle(moving ? REXBOX::PARTICLE_CARFLAME_MOVING : REXBOX::PARTICLE_CARFLAME,
		position, velocity, nil, moving ? 0.8f * 1.4f : 0.8f, 0,
		int32(CGeneral::GetRandomNumberInRange(0.0f, 20.0f)), 0, 0);
}

bool ParticleEx::AddFire(CVector position, CEntity *entity)
{
	const float strength = 0.8f * 0.7f;
	float size = strength;
	CVector velocity(0.0f, 0.0f, CGeneral::GetRandomNumberInRange(strength / 80.0f, strength / 10.0f) * 0.35f);
	bool moving = false;
	if (entity && entity->IsVehicle()) {
		CVehicle *vehicle = static_cast<CVehicle *>(entity);
		if (vehicle->IsCar()) position.z -= 0.15f;
		moving = (SQR(vehicle->m_vecMoveSpeed.x) + SQR(vehicle->m_vecMoveSpeed.y)) > 0.003f;
		position.x += CGeneral::GetRandomNumberInRange(-0.25f, 0.25f);
		position.y += CGeneral::GetRandomNumberInRange(-0.25f, 0.25f);
	} else if (!entity) {
		// Fix the Xbox molotov offset: distribute flames around the fire centre.
		position.x += CGeneral::GetRandomNumberInRange(-1.0f, 1.0f);
		position.y += CGeneral::GetRandomNumberInRange(-1.0f, 1.0f);
		position.z -= 0.5f;
		size *= 2.5f;
		velocity.z *= 3.7f;
	}
	if (moving) {
		size *= 1.4f;
		position.z += 0.2f;
	}
	REXBOX::ParticleEngine::AddParticle(moving ? REXBOX::PARTICLE_CARFLAME_MOVING : REXBOX::PARTICLE_CARFLAME,
		position, velocity, nil, size, 0, int32(CGeneral::GetRandomNumberInRange(0.0f, 20.0f)), 0, 0);
	return moving;
}

void ParticleEx::ConfigureObject(CParticleObject *object)
{
	// Also normalize saved emitters when loading a save made with a different system.
	switch (object->m_Type) {
	case POBJECT_PAVEMENT_STEAM:
	case POBJECT_WALL_STEAM:
	case POBJECT_FIRE_TRAIL:
		object->m_nSkipFrames = UsesPS2Emitters() ? 1 : 3;
		break;
	case POBJECT_DARK_SMOKE:
		object->m_ParticleType = ActiveSystem == Xbox ? PARTICLE_ENGINE_SMOKE2 : PARTICLE_STEAM_NY;
		object->m_nNumEffectCycles = ActiveSystem == Xbox ? 4 : 1;
		object->m_nSkipFrames = UsesPS2Emitters() ? 1 : 3;
		object->m_Color = ActiveSystem == Xbox ? CRGBA(0, 0, 0, 0) : CRGBA(16, 16, 16, 255);
		break;
	case POBJECT_CAR_WATER_SPLASH:
	case POBJECT_PED_WATER_SPLASH:
		object->m_nSkipFrames = ActiveSystem == PS2 ? 3 : 1;
		break;
	case POBJECT_SPLASHES_AROUND:
		object->m_nNumEffectCycles = ActiveSystem == PS2 ? 30 : 15;
		break;
	case POBJECT_SMALL_FIRE:
	case POBJECT_BIG_FIRE:
		object->m_nSkipFrames = UsesPS2Emitters() ? 1 : 2;
		break;
	default:
		break;
	}
}

// Hybrid keeps PS2 smoke, dust and scene effects, with Xbox fire and water.
ParticleEx::System ParticleEx::SystemForParticle(::tParticleType type)
{
	if (ActiveSystem != PS2Xbox) return ActiveSystem;
	switch (type) {
	case PARTICLE_FLAME: case PARTICLE_FIREBALL: case PARTICLE_CARFLAME:
	case PARTICLE_EXPLOSION_MEDIUM: case PARTICLE_EXPLOSION_LARGE:
	case PARTICLE_EXPLOSION_MFAST: case PARTICLE_EXPLOSION_LFAST:
	case PARTICLE_WATER: case PARTICLE_SPLASH:
	case PARTICLE_RAIN_SPLASH: case PARTICLE_RAIN_SPLASH_BIGGROW:
	case PARTICLE_RAIN_SPLASHUP: case PARTICLE_WATERSPRAY:
	case PARTICLE_CAR_SPLASH: case PARTICLE_PED_SPLASH:
	case PARTICLE_BOAT_SPLASH: case PARTICLE_BOAT_THRUSTJET: case PARTICLE_BOAT_WAKE:
	case PARTICLE_WATER_HYDRANT: case PARTICLE_WATER_CANNON:
		return Xbox;
	default:
		return PS2;
	}
}
