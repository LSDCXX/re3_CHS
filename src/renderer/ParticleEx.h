#pragma once
#include "ParticlePS2.h"
#include "ParticleXbox.h"

class CParticleObject;

// Changed in re3.ini; selection takes effect at the next game startup.
namespace ParticleEx {
enum System { PC = 0, PS2 = 1, Xbox = 2 };
extern int32 SelectedSystem;
extern System ActiveSystem;
void Initialise();
void Shutdown();
REXBOX::tParticleType XboxType(::tParticleType type);
bool AddFire(CVector position, CEntity *entity);
void AddVehicleFire(CVector position, const CVector &speed);
void ConfigureObject(CParticleObject *object);
}
