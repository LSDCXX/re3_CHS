#pragma once
#include "ParticlePS2.h"
#include "ParticleXbox.h"

class CParticleObject;

// Changed in re3.ini; selection takes effect at the next game startup.
namespace ParticleEx {
enum System { PC = 0, PS2 = 1, Xbox = 2, PS2Xbox = 3 };
extern int32 SelectedSystem;
extern System ActiveSystem;
inline bool UsesXboxFire() { return ActiveSystem == Xbox || ActiveSystem == PS2Xbox; }
inline bool UsesPS2Emitters() { return ActiveSystem == PS2 || ActiveSystem == PS2Xbox; }
System SystemForParticle(::tParticleType type);
void Initialise();
void Shutdown();
REXBOX::tParticleType XboxType(::tParticleType type);
bool AddFire(CVector position, CEntity *entity);
void AddVehicleFire(CVector position, const CVector &speed);
void ConfigureObject(CParticleObject *object);
}
