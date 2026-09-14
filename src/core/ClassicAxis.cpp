#include "common.h"
#include "ClassicAxis.h"
#include "AnimBlendAssociation.h"
#include "AnimManager.h"
#include "Camera.h"
#include "CutsceneMgr.h"
#include "FileMgr.h"
#include "Frontend.h"
#include "General.h"
#include "Pad.h"
#include "PlayerPed.h"
#include "Replay.h"
#include "RpAnimBlend.h"
#include "Timer.h"
#include "WeaponInfo.h"
#include "World.h"
#include <cmath>
#include <string>

namespace
{
bool configured, forceAutoAim, ownsDuck, ownsAim, changedCrosshair;
bool modernCamera = true, zoomRifles = true, storiesAim = false, storiesArm = false;
float oldCrossX, oldCrossY;
float aimBlend, duckOffset;
uint32 nextShot;
eWeaponType shotWeapon = WEAPONTYPE_UNARMED;
bool shotScheduled, ownsAttack;
bool controllerFreeAim;
char crouchKey = 'C';
std::string walkKey = "LALT";
struct WalkButton { const char *name; bool (CPad::*held)(); };
const WalkButton walkButtons[] = {
	{"LALT", &CPad::GetLeftAlt}, {"RALT", &CPad::GetRightAlt},
	{"LSHIFT", &CPad::GetLeftShift}, {"RSHIFT", &CPad::GetRightShift},
	{"LCTRL", &CPad::GetLeftCtrl}, {"RCTRL", &CPad::GetRightCtrl},
	{"TAB", &CPad::GetTab}, {"CAPSLOCK", &CPad::GetCapsLock},
	{"ENTER", &CPad::GetEnter}, {"BACKSPACE", &CPad::GetBackspace}
};
std::string
Trim(const std::string &s)
{
	size_t a = s.find_first_not_of(" \t\r\n");
	return a == std::string::npos ? "" : s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}
void
SetWalkKey(std::string key)
{
	key = Trim(key);
	for(char &c : key) if(c >= 'a' && c <= 'z') c -= 'a' - 'A';
	if(key == "SPACE") key = " ";
	if(key == "LCONTROL") key = "LCTRL";
	if(key == "RCONTROL") key = "RCTRL";
	if(key == "CONTROL") key = "CTRL";
	bool valid = key == "NULL" || key == "ALT" || key == "SHIFT" || key == "CTRL" ||
	             (key.size() == 1 && (key[0] == ' ' || (key[0] >= 'A' && key[0] <= 'Z') || (key[0] >= '0' && key[0] <= '9')));
	for(const auto &button : walkButtons) if(key == button.name) valid = true;
	walkKey = valid ? key : "LALT";
}
bool
WalkHeld(CPad *pad)
{
	if(walkKey == "NULL") return false;
	if(walkKey.size() == 1) return pad->GetChar((unsigned char)walkKey[0]);
	if(walkKey == "ALT") return pad->GetLeftAlt() || pad->GetRightAlt();
	if(walkKey == "SHIFT") return pad->GetLeftShift() || pad->GetRightShift();
	if(walkKey == "CTRL") return pad->GetLeftCtrl() || pad->GetRightCtrl();
	for(const auto &button : walkButtons) if(walkKey == button.name) return (pad->*button.held)();
	return false;
}
void
Settings()
{
	if(configured) return;
	configured = true;
	std::string path = std::string(CFileMgr::GetRootDirName()) + "data\\ClassicAxisIII.ini";
	int f = CFileMgr::OpenFile(path.c_str(), "rb");
	if(!f) return;
	char buf[256];
	while(CFileMgr::ReadLine(f, buf, sizeof(buf))) {
		std::string s = buf;
		s = s.substr(0, s.find(';'));
		size_t eq = s.find('=');
		if(eq == std::string::npos) continue;
		std::string k = Trim(s.substr(0, eq)), v = Trim(s.substr(eq + 1));
		bool yes = v == "true" || v == "1";
		if(k == "ForceAutoAim")
			forceAutoAim = yes;
		else if(k == "ModernCamera")
			modernCamera = yes;
		else if(k == "ZoomForAssaultRifles")
			zoomRifles = yes;
		else if(k == "StoriesAimingCoords")
			storiesAim = yes;
		else if(k == "StoriesPointingArm")
			storiesArm = yes;
		else if(k == "WalkKey")
			SetWalkKey(v);
		else if(k == "CrouchKey")
			crouchKey = v.size() == 1 ? v[0] : 0;
	}
	CFileMgr::CloseFile(f);
}
bool
UsingController()
{
#ifdef DETECT_PAD_INPUT_SWITCH
	return CPad::IsAffectedByController;
#else
	return false;
#endif
}
bool
Supported(CPed *p)
{
	eWeaponType w = p->GetWeapon()->m_eWeaponType;
	return (w >= WEAPONTYPE_COLT45 && w <= WEAPONTYPE_M16) || w == WEAPONTYPE_FLAMETHROWER;
}
void
Stand(CPlayerPed *p)
{
	if(p->m_nPedState == PED_AIM_GUN) p->ClearPointGunAt();
	for(int id : {ANIM_STD_DUCK_DOWN, ANIM_STD_DUCK_LOW, ANIM_STD_RBLOCK_SHOOT}) {
		CAnimBlendAssociation *a = RpAnimBlendClumpGetAssociation(p->GetClump(), id);
		if(a) {
			a->flags |= ASSOC_DELETEFADEDOUT;
			a->blendDelta = -4.0f;
		}
	}
	p->bIsDucking = false;
	p->bCrouchWhenShooting = false;
	p->m_duckTimer = 0;
	ownsDuck = false;
}
} // namespace
bool
CClassicAxis::Enabled()
{
	Settings();
	return CMenuManager::m_ControlMethod == CONTROL_STANDARD;
}
bool
CClassicAxis::Active(const CPed *ped)
{
	return Enabled() && ped && ped == FindPlayerPed() && !ped->bInVehicle && TheCamera.m_bLookingAtPlayer &&
	       TheCamera.WhoIsInControlOfTheCamera == CAMCONTROL_GAME && !CCutsceneMgr::IsRunning() && !TheCamera.m_WideScreenOn && !CReplay::IsPlayingBack() &&
	       TheCamera.Cams[TheCamera.ActiveCam].Mode == CCam::MODE_FOLLOWPED &&
	       (ped->m_nPedState == PED_IDLE || ped->m_nPedState == PED_NONE || ped->m_nPedState == PED_ATTACK || ped->m_nPedState == PED_AIM_GUN ||
	        ped->m_nPedState == PED_FIGHT || ped->m_nPedState == PED_JUMP);
}
bool
CClassicAxis::Aiming(const CPed *ped)
{
	if(!Active(ped) || ped->m_nPedState == PED_JUMP) return false;
	CPad *pad = CPad::GetPad(0);
	CPed *p = const_cast<CPed *>(ped);
	return !pad->ArePlayerControlsDisabled() && !CTimer::GetIsPaused() && pad->GetTarget() && !pad->GetSprint() && !pad->JumpJustDown() && Supported(p) &&
	       p->GetWeapon()->m_nAmmoTotal > 0;
}
bool
CClassicAxis::AutoAim()
{ return Enabled() && (UsingController() || forceAutoAim); }
bool
CClassicAxis::Crouched(const CPed *ped)
{ return Active(ped) && ownsDuck && ped->bIsDucking; }
bool
CClassicAxis::Walking(const CPed *ped)
{ return Active(ped) && WalkHeld(CPad::GetPad(0)); }
float
CClassicAxis::MoveLimit(const CPed *ped)
{
	if(!Active(ped)) return 100.0f;
	if(Crouched(ped)) return 0.0f;
	if(WalkHeld(CPad::GetPad(0))) return 1.0f;
	if(Aiming(ped)) {
		eWeaponType weapon = const_cast<CPed *>(ped)->GetWeapon()->m_eWeaponType;
		// One-handed weapons keep the normal movement range while aiming.
		// Only two-handed aiming is limited to walking.
		if(weapon != WEAPONTYPE_COLT45 && weapon != WEAPONTYPE_UZI) return 1.0f;
	}
	return 100.0f;
}
void
CClassicAxis::Reset(CPlayerPed *ped)
{
	if(ped) {
		if(ownsDuck) Stand(ped);
		if(ownsAim) {
			if(ped->m_nPedState == PED_AIM_GUN) ped->ClearPointGunAt();
			ped->ClearLookFlag();
			ped->ClearAimFlag();
			ped->bIsPointingGunAt = false;
			if(ped->m_pPointGunAt) ped->ClearWeaponTarget();
		}
	}
	ownsDuck = false;
	ownsAim = false;
	aimBlend = 0.0f;
	duckOffset = 0.0f;
	shotScheduled = false;
	controllerFreeAim = false;
	ownsAttack = false;
	if(changedCrosshair) {
		TheCamera.m_f3rdPersonCHairMultX = oldCrossX;
		TheCamera.m_f3rdPersonCHairMultY = oldCrossY;
	}
	changedCrosshair = false;
}
void
CClassicAxis::Update(CPlayerPed *p)
{
	bool active = Active(p);
	CPad *pad = CPad::GetPad(0);
	if(CTimer::GetIsPaused()) return;
	if(!Aiming(p) || !CPad::IsStandardControls()) controllerFreeAim = false;
	if(ownsAim && !Aiming(p)) {
		if(p->m_nPedState == PED_AIM_GUN)
			p->ClearPointGunAt();
		else {
			p->ClearLookFlag();
			p->ClearAimFlag();
			p->bIsPointingGunAt = false;
		}
		if(p->m_pPointGunAt) p->ClearWeaponTarget();
		ownsAim = false;
	}
	if(!active || pad->ArePlayerControlsDisabled()) {
		ownsAttack = false;
		if(ownsDuck) Stand(p);
		if(changedCrosshair) {
			TheCamera.m_f3rdPersonCHairMultX = oldCrossX;
			TheCamera.m_f3rdPersonCHairMultY = oldCrossY;
			changedCrosshair = false;
		}
		return;
	}
	if(!changedCrosshair) {
		oldCrossX = TheCamera.m_f3rdPersonCHairMultX;
		oldCrossY = TheCamera.m_f3rdPersonCHairMultY;
		changedCrosshair = true;
	}
	// The crosshair and both native bullet-ray implementations share these values.
	// Ignore the old INI offsets: the reticle must stay at the screen centre.
	TheCamera.m_f3rdPersonCHairMultX = 0.5f;
	TheCamera.m_f3rdPersonCHairMultY = 0.5f;
	bool toggle = (crouchKey && pad->GetCharJustDown(crouchKey)) ||
	              (CPad::IsStandardControls() && pad->NewState.LeftShock && !pad->OldState.LeftShock);
	if(ownsDuck && (toggle || pad->GetSprint() || pad->JumpJustDown() || p->m_nPedState == PED_JUMP || pad->ExitVehicleJustDown() || (!Supported(p) && pad->GetWeapon())))
		Stand(p);
	else if(toggle && !p->bIsDucking && p->m_nPedState != PED_JUMP && !pad->GetSprint() && !pad->JumpJustDown() && !pad->ExitVehicleJustDown()) {
		if(p->m_nPedState == PED_AIM_GUN) p->ClearPointGunAt();
		CAnimManager::BlendAnimation(p->GetClump(), ASSOCGRP_STD, ANIM_STD_DUCK_DOWN, 4.0f);
		p->bIsDucking = true;
		ownsDuck = true;
	}
	if(ownsDuck) {
		p->m_duckTimer = CTimer::GetTimeInMilliseconds() + 60000;
		p->bCrouchWhenShooting = true;
		if(!Aiming(p) && p->m_nPedState != PED_ATTACK) {
			CAnimBlendAssociation *duck = RpAnimBlendClumpGetAssociation(p->GetClump(), ANIM_STD_DUCK_DOWN);
			if(!duck || duck->blendDelta < 0.0f) CAnimManager::BlendAnimation(p->GetClump(), ASSOCGRP_STD, ANIM_STD_DUCK_DOWN, 4.0f);
		}
	}
	if(Aiming(p)) {
		bool startedAiming = !ownsAim;
		ownsAim = true;
		bool autoAim = AutoAim();
		bool standardPad = CPad::IsStandardControls();
		// Manual input owns the remainder of this aim session, even after the
		// stick returns to centre. Release LT to permit a fresh automatic lock.
		if(standardPad && (pad->LookAroundLeftRight() || pad->LookAroundUpDown()))
			controllerFreeAim = true;
		bool manualLook = standardPad ? (!pad->GetStandardLockOn() || controllerFreeAim) :
		                  ((UsingController() && (pad->LookAroundLeftRight() || pad->LookAroundUpDown())) ||
		                   fabsf(pad->GetMouseX()) > 1.0f || fabsf(pad->GetMouseY()) > 1.0f);
		if(p->m_pPointGunAt && (!autoAim || manualLook ||
		   (p->m_pPointGunAt->GetPosition() - p->GetPosition()).Magnitude() < 0.5f))
			p->ClearWeaponTarget();
		float heading = TheCamera.Cams[TheCamera.ActiveCam].Front.Heading();
		bool targetDied = standardPad && autoAim && !manualLook && p->m_pPointGunAt && p->m_pPointGunAt->IsPed() &&
		                  (static_cast<CPed *>(p->m_pPointGunAt)->DyingOrDead() || static_cast<CPed *>(p->m_pPointGunAt)->m_fHealth <= 0.0f);
		if(targetDied) {
			p->ClearWeaponTarget();
			p->FindNearestWeaponLockOnTarget();
		}
		// Acquire relative to the camera, including when Free Cam was facing away from the player.
		bool shiftLeft = pad->ShiftTargetLeftJustDown();
		bool shiftRight = pad->ShiftTargetRightJustDown();
		// Acquire when aiming starts, including entry after another state delayed
		// the original button edge. Target-switch buttons can explicitly reacquire.
		if(!targetDied && autoAim && !manualLook && !p->m_pPointGunAt &&
		   ((standardPad && pad->GetStandardLockOn()) ||
		    (UsingController() && (startedAiming || pad->TargetJustDown() || shiftLeft || shiftRight)) ||
		    (forceAutoAim && pad->TargetJustDown()))) {
			p->m_fRotationCur = p->m_fRotationDest = heading;
			p->SetHeading(heading);
			p->FindWeaponLockOnTarget();
		} else if(!targetDied && autoAim && !manualLook && p->m_pPointGunAt) {
			if(shiftLeft) p->FindNextWeaponLockOnTarget(p->m_pPointGunAt, true);
			else if(shiftRight) p->FindNextWeaponLockOnTarget(p->m_pPointGunAt, false);
		}
		if(autoAim && p->m_pPointGunAt) heading = (p->m_pPointGunAt->GetPosition() - p->GetPosition()).Heading();
		p->m_fRotationCur = p->m_fRotationDest = heading;
		p->SetHeading(heading);
#ifdef FREE_CAM
		p->m_cachedCamSource = TheCamera.Cams[TheCamera.ActiveCam].Source;
		p->m_cachedCamFront = TheCamera.Cams[TheCamera.ActiveCam].Front;
		p->m_cachedCamUp = TheCamera.Cams[TheCamera.ActiveCam].Up;
#endif
		p->m_lookTimer = 0;
		if(p->m_pPointGunAt) {
			p->SetLookFlag(p->m_pPointGunAt, true);
			p->SetAimFlag(p->m_pPointGunAt);
		} else {
			p->SetLookFlag(heading, true);
			p->SetAimFlag(heading);
		}
		p->m_fFPSMoveHeading = Clamp(TheCamera.Find3rdPersonQuickAimPitch(), -DEGTORAD(45.f), DEGTORAD(45.f));
		if(storiesArm && CWeaponInfo::GetWeaponInfo(p->GetWeapon()->m_eWeaponType)->IsFlagSet(WEAPONFLAG_CANAIM_WITHARM))
			p->m_fFPSMoveHeading -= DEGTORAD(8.f);
#ifdef FREE_CAM
		p->m_bFreeAimActive = !p->m_pPointGunAt;
#endif
		if(p->m_nPedState != PED_ATTACK && p->m_nPedState != PED_AIM_GUN && !RpAnimBlendClumpGetAssociation(p->GetClump(), ANIM_STD_HGUN_RELOAD) &&
		   !RpAnimBlendClumpGetAssociation(p->GetClump(), ANIM_STD_AK_RELOAD))
			p->SetPointGunAt(p->m_pPointGunAt);
	}
}
void
CClassicAxis::Camera(CCam &cam, CVector &target, float &distance)
{
	CPed *p = static_cast<CPed *>(cam.CamTargetEntity);
	if(!Active(p)) return;
	bool aim = Aiming(p);
	float dt = Clamp(CTimer::GetTimeStep(), 0.f, 3.f);
	float blend = 1.0f - powf(0.85f, dt);
	aimBlend += ((aim ? 1.0f : 0.0f) - aimBlend) * blend;
	duckOffset += ((Crouched(p) ? -0.6f : 0.0f) - duckOffset) * blend;
	float desired = 70.f;
	eWeaponType weapon = p->GetWeapon()->m_eWeaponType;
	if(aim && zoomRifles && (weapon == WEAPONTYPE_AK47 || weapon == WEAPONTYPE_M16)) desired = 50.f;
	cam.FOV += (desired - cam.FOV) * (1.0f - powf(0.9f, dt));
	distance += (2.7f - distance) * aimBlend;
	CVector right(Sin(cam.Beta), -Cos(cam.Beta), 0);
	// Shift the aiming camera to the right so the player occupies the left side.
	// Ordinary movement, including jumping, has no lateral offset.
	float shoulder = storiesAim ? 0.85f : (modernCamera ? 0.65f : 0.55f);
	target += right * (shoulder * aimBlend);
	target.z += duckOffset;
	if(aim) {
		cam.Alpha = Clamp(cam.Alpha, -DEGTORAD(50.f), DEGTORAD(50.f));
		if(AutoAim() && p->m_pPointGunAt) {
			CVector delta = p->m_pPointGunAt->GetPosition() - target;
			cam.Beta = CGeneral::GetATanOfXY(delta.x, delta.y);
			cam.Alpha = atan2f(delta.z, delta.Magnitude2D());
		}
	}
}
void
CClassicAxis::AdjustWeaponInfo(CPed *ped, CWeaponInfo &info)
{
	if(!Crouched(ped) || !Supported(ped)) return;
	info.m_AnimToPlay = info.m_Anim2ToPlay = ANIM_STD_RBLOCK_SHOOT;
	// Pose/recoil timing only. Actual shots use the original weapon cadence below.
	info.m_fAnimLoopStart = 12.f / 30.f;
	info.m_fAnimLoopEnd = 15.f / 30.f;
	info.m_fAnimFrameFire = 14.f / 30.f;
	info.m_fAnim2FrameFire = info.m_fAnimFrameFire;
	info.m_Flags &= ~WEAPONFLAG_CANAIM_WITHARM;
}

bool
CClassicAxis::BeginAttack(CPlayerPed *ped)
{
	// Called by the real weapon-input path, not just CPed::Attack. A crouched
	// player can already be in PED_AIM_GUN when the fire button is first pressed.
	if(!Aiming(ped) || !ped->IsPedInControl() || !CPad::GetPad(0)->GetWeapon()) return false;
	if(ped->m_nPedState != PED_ATTACK) {
		if(ped->m_nPedState != PED_AIM_GUN) ped->SetStoredState();
		ped->SetPedState(PED_ATTACK);
	}
	return ProcessAttack(ped);
}

bool
CClassicAxis::ProcessAttack(CPed *ped)
{
	if(!Active(ped) || !Supported(ped)) return false;
	if(!Crouched(ped) && !Aiming(ped) && ped->GetWeapon()->m_eWeaponType != WEAPONTYPE_FLAMETHROWER && !ownsAttack) return false;
	ownsAttack = true;
	CPad *pad = CPad::GetPad(0);
	if(CTimer::GetIsPaused()) return true;
	if(!pad->GetWeapon() || pad->ArePlayerControlsDisabled()) {
		ped->bIsAttacking = false;
		ped->ClearAttack();
		ownsAttack = false;
		return true;
	}
	ped->bIsAttacking = true;
	CWeapon *weapon = ped->GetWeapon();
	const CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(weapon->m_eWeaponType);
	if(weapon->m_eWeaponState == WEAPONSTATE_RELOADING || weapon->m_nAmmoInClip <= 0) return true;
	uint32 now = CTimer::GetTimeInMilliseconds();
	if(shotScheduled && shotWeapon == weapon->m_eWeaponType && (int32)(now - nextShot) < 0) return true;

	CVector firePos = info->m_vecFireOffset;
	ped->TransformToNode(firePos, PED_HANDR);
	if(weapon->Fire(ped, &firePos)) {
		// weapon.dat's animation period controls normal on-foot fire (the generic
		// firing-rate field is 1 ms for rifles and does not describe shotgun pumping).
		float period = info->m_fAnimLoopEnd - info->m_fAnimLoopStart;
		uint32 interval = period > 0.0f ? (uint32)ceilf(period * 1000.0f) : info->m_nFiringRate;
		nextShot = now + Max(interval, 1u);
		shotWeapon = weapon->m_eWeaponType;
		shotScheduled = true;
		CWeaponInfo pose = *info;
		AdjustWeaponInfo(ped, pose);
		CAnimBlendAssociation *recoil = CAnimManager::BlendAnimation(ped->GetClump(), ASSOCGRP_STD, pose.m_AnimToPlay, 8.0f);
		recoil->Start(pose.m_fAnimLoopStart);
		if(!Crouched(ped) && weapon->m_eWeaponState == WEAPONSTATE_RELOADING) {
			if(pose.m_AnimToPlay == ANIM_STD_WEAPON_HGUN_BODY)
				CAnimManager::BlendAnimation(ped->GetClump(), ASSOCGRP_STD, ANIM_STD_HGUN_RELOAD, 8.0f);
			else if(pose.m_AnimToPlay == ANIM_STD_WEAPON_AK_BODY)
				CAnimManager::BlendAnimation(ped->GetClump(), ASSOCGRP_STD, ANIM_STD_AK_RELOAD, 8.0f);
		}
	}
	// CWeapon::Fire/Update retain ownership of ammo, damage, sound and reloads.
	return true;
}
