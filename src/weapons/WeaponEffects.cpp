#include "common.h"

#include "main.h"
#include "Camera.h"
#include "ClassicAxis.h"
#include "PlayerPed.h"
#include "Sprite2d.h"
#include "Timer.h"
#include "World.h"
#include "WeaponEffects.h"
#include "TxdStore.h"
#include "Sprite.h"

RwTexture *gpCrossHairTex;
RwRaster *gpCrossHairRaster;

CWeaponEffects gCrossHair;

CWeaponEffects::CWeaponEffects()
{
	
}

CWeaponEffects::~CWeaponEffects()
{
	
}

void
CWeaponEffects::Init(void)
{
	gCrossHair.m_bActive = false;
	gCrossHair.m_vecPos = CVector(0.0f, 0.0f, 0.0f);
	gCrossHair.m_nRed = 0;
	gCrossHair.m_nGreen = 0;
	gCrossHair.m_nBlue = 0;
	gCrossHair.m_nAlpha = 255;
	gCrossHair.m_fSize = 1.0f;
	gCrossHair.m_fRotation = 0.0f;
	
	
	CTxdStore::PushCurrentTxd();
	int32 slot = CTxdStore::FindTxdSlot("particle");
	CTxdStore::SetCurrentTxd(slot);
	
	gpCrossHairTex    = RwTextureRead("crosshair", nil);
	gpCrossHairRaster = RwTextureGetRaster(gpCrossHairTex);
	
	CTxdStore::PopCurrentTxd();
}

void
CWeaponEffects::Shutdown(void)
{
	RwTextureDestroy(gpCrossHairTex);
#if GTA_VERSION >= GTA3_PC_11
	gpCrossHairTex = nil;
#endif
}

void
CWeaponEffects::MarkTarget(CVector pos, uint8 red, uint8 green, uint8 blue, uint8 alpha, float size)
{
	gCrossHair.m_bActive = true;
	gCrossHair.m_vecPos = pos;
	gCrossHair.m_nRed = red;
	gCrossHair.m_nGreen = green;
	gCrossHair.m_nBlue = blue;
	gCrossHair.m_nAlpha = alpha;
	gCrossHair.m_fSize = size;
}

void
CWeaponEffects::ClearCrossHair(void)
{
	gCrossHair.m_bActive = false;
}

static void
DrawAxisTriangle(float x, float y, float radius, float angle, float size, const CRGBA &color)
{
	float dx = Cos(angle), dy = Sin(angle);
	float cx = x + dx * radius, cy = y + dy * radius;
	float tipX = cx - dx * size, tipY = cy - dy * size;
	float baseX = cx + dx * size * 0.5f, baseY = cy + dy * size * 0.5f;
	CSprite2d::Draw2DPolygon(tipX, tipY, baseX - dy * size * 0.55f, baseY + dx * size * 0.55f,
		baseX + dy * size * 0.55f, baseY - dx * size * 0.55f, tipX, tipY, color);
}

static bool
RenderAxisLockOn()
{
	CPlayerPed *player = FindPlayerPed();
	if(!gCrossHair.m_bActive || !CClassicAxis::Aiming(player) || CClassicAxis::Options.LockOnTargetType == 0)
		return false;
	RwV3d pos;
	float w, h;
	if(!CSprite::CalcScreenCoors(gCrossHair.m_vecPos, &pos, &w, &h, true)) return true;
	float health = player->m_pPointGunAt && player->m_pPointGunAt->IsPed()
		? Clamp(static_cast<CPed *>(player->m_pPointGunAt)->m_fHealth / 100.0f, 0.0f, 1.0f) : 1.0f;
	CRGBA color = CClassicAxis::Options.LockOnTargetType == 1
		? CRGBA(uint8(255.0f * (1.0f - health)), uint8(255.0f * health), 0, 255)
		: CRGBA(0, uint8(255.0f * health), 0, 150);
	float radius = Clamp(w * 0.16f, SCREEN_SCALE_Y(12.0f), SCREEN_SCALE_Y(28.0f));
	float size = CClassicAxis::Options.LockOnTargetType == 1 ? SCREEN_SCALE_Y(8.0f) : SCREEN_SCALE_Y(6.0f);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
	float phase = CClassicAxis::Options.LockOnTargetType == 1 ? CTimer::GetTimeInMilliseconds() * 0.001f : 0.0f;
	for(int i = 0; i < 3; i++) {
		float angle = CClassicAxis::Options.LockOnTargetType == 1 ? phase + DEGTORAD(90.0f + i * 120.0f) : DEGTORAD(i * 90.0f);
		DrawAxisTriangle(pos.x, pos.y, radius, angle, size, color);
	}
	return true;
}

static void
RenderAxisMouseTarget()
{
	if(!CClassicAxis::Options.ShowTriangleForMouseRecruit) return;
	CPed *target = CClassicAxis::MouseTarget();
	if(!target) return;
	CVector world;
	target->m_pedIK.GetComponentPosition(world, PED_MID);
	world.z += 1.0f;
	RwV3d pos;
	float w, h;
	if(!CSprite::CalcScreenCoors(world, &pos, &w, &h, false)) return;
	float health = Clamp(target->m_fHealth / 100.0f, 0.0f, 1.0f);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
	DrawAxisTriangle(pos.x, pos.y, 0.0f, DEGTORAD(-90.0f), SCREEN_SCALE_Y(10.0f),
		CRGBA(uint8(255.0f * (1.0f - health)), uint8(255.0f * health), 0, 150));
}

void
CWeaponEffects::Render(void)
{
	bool axisMarker = RenderAxisLockOn();
	if ( gCrossHair.m_bActive && !axisMarker )
	{
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      (void *)FALSE);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
		RwRenderStateSet(rwRENDERSTATESRCBLEND,          (void *)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND,         (void *)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER,     (void *)gpCrossHairRaster);

		RwV3d pos;
		float w, h;
		if ( CSprite::CalcScreenCoors(gCrossHair.m_vecPos, &pos, &w, &h, true) )
		{
			PUSH_RENDERGROUP("CWeaponEffects::Render");

			float recipz = 1.0f / pos.z;
			CSprite::RenderOneXLUSprite(pos.x, pos.y, pos.z,
				gCrossHair.m_fSize * w, gCrossHair.m_fSize * h,
				gCrossHair.m_nRed, gCrossHair.m_nGreen, gCrossHair.m_nBlue, 255,
				recipz, 255);

			POP_RENDERGROUP();
		}
		
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,      (void *)FALSE);
		RwRenderStateSet(rwRENDERSTATESRCBLEND,          (void *)rwBLENDSRCALPHA);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND,         (void *)rwBLENDINVSRCALPHA);
	}
	RenderAxisMouseTarget();
}
