#pragma once
class CPed;
class CPlayerPed;
class CCam;
class CVector;
class CWeaponInfo;

// Native Classic Axis integration. Original controls are never overwritten.
class CClassicAxis
{
public:
	static bool Enabled();
	static bool Active(const CPed *ped);
	static bool Aiming(const CPed *ped);
	static bool AutoAim();
	static bool Crouched(const CPed *ped);
	static float MoveLimit(const CPed *ped);
	static bool Walking(const CPed *ped);
	static void Update(CPlayerPed *ped);
	static void Reset(CPlayerPed *ped = nullptr);
	static void Camera(CCam &cam, CVector &target, float &distance);
	static void AdjustWeaponInfo(CPed *ped, CWeaponInfo &info);
	static bool ProcessAttack(CPed *ped);
	static bool BeginAttack(CPlayerPed *ped);
};
