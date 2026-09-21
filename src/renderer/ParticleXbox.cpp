// Native re3 adaptation of ParticleEx by Fire_Head. See docs/PARTICLEEX.md.
#include "ParticleXbox.h"
#include "ParticleEx.h"
#include "ParticleExCompat.h"


#define MAX_PARTICLES_ON_SCREEN   (1000)


//(5)
#define MAX_SMOKE_FILES           ARRAY_SIZE(SmokeFiles)

//(5)
#define MAX_SMOKE2_FILES          ARRAY_SIZE(Smoke2Files)
//(5)
#define MAX_RUBBER_FILES          ARRAY_SIZE(RubberFiles)
//(5)
#define MAX_RAINSPLASH_FILES      ARRAY_SIZE(RainSplashFiles)
//(3)
#define MAX_WATERSPRAY_FILES      ARRAY_SIZE(WatersprayFiles)
//(6)
#define MAX_EXPLOSIONMEDIUM_FILES ARRAY_SIZE(ExplosionMediumFiles)
//(4)
#define MAX_GUNFLASH_FILES        ARRAY_SIZE(GunFlashFiles)
//(2)
#define MAX_RAINSPLASHUP_FILES    ARRAY_SIZE(RainSplashupFiles)
//(4)
#define MAX_BIRDFRONT_FILES       ARRAY_SIZE(BirdfrontFiles)
//(4)
#define MAX_CARDEBRIS_FILES       ARRAY_SIZE(CardebrisFiles)
//(4)
#define MAX_CARSPLASH_FILES       ARRAY_SIZE(CarsplashFiles)

//(4)
#define MAX_RAINDROP_FILES        ARRAY_SIZE(RaindropFiles)

//(45)
#define MAX_CARFLAME_FILES        ARRAY_SIZE(CarFlameFiles)

#define Kprintf(...)


namespace REXBOX
{

const char SmokeFiles[][6+1] =
{
	"smoke1",
	"smoke2",
	"smoke3",
	"smoke4",
	"smoke5"
};


const char Smoke2Files[][9+1] =
{
	"smokeII_1",
	"smokeII_2",
	"smokeII_3",
	"smokeII_4",
	"smokeII_5"
};

const char RubberFiles[][7+1] =
{
	"rubber1",
	"rubber2",
	"rubber3",
	"rubber4",
	"rubber5"
};

const char RainSplashFiles[][7+1] =
{
	"splash1",
	"splash2",
	"splash3",
	"splash4",
	"splash5"
};

const char WatersprayFiles[][11+1] =
{
	"waterspray1",
	"waterspray2",
	"waterspray3"
};

const char ExplosionMediumFiles[][7+1] =
{
	"explo01",
	"explo02",
	"explo03",
	"explo04",
	"explo05",
	"explo06",
	"explo07",
	"explo08",
	"explo09",
	"explo10",
	"explo11",
	"explo12",
	"explo13",
	"explo14",
	"explo15"
};

const char GunFlashFiles[][9+1] =
{
	"gunflash1",
	"gunflash2",
	"gunflash3",
	"gunflash4"
};

const char RaindropFiles[][9+1] =
{
	"raindrop1",
	"raindrop2",
	"raindrop3",
	"raindrop4"
};

const char RainSplashupFiles[][10+1] =
{
	"splash_up1",
	"splash_up2"
};

const char BirdfrontFiles[][8+1] =
{
	"birdf_01",
	"birdf_02",
	"birdf_03",
	"birdf_04"
};

const char CardebrisFiles[][12+1] =
{
	"cardebris_01",
	"cardebris_02",
	"cardebris_03",
	"cardebris_04"
};

const char CarsplashFiles[][12+1] =
{
	"carsplash_01",
	"carsplash_02",
	"carsplash_03",
	"carsplash_04"
};


const char CarFlameFiles[][7+1] =
{
	"flame01",
	"flame02",
	"flame03",
	"flame04",
	"flame05",
	"flame06",
	"flame07",
	"flame08",
	"flame09",
	"flame10",
	"flame11",
	"flame12",
	"flame13",
	"flame14",
	"flame15",
	"flame16",
	"flame17",
	"flame18",
	"flame19",
	"flame20",
	"flame21",
	"flame22",
	"flame23",
	"flame24",
	"flame25",
	"flame26",
	"flame27",
	"flame28",
	"flame29",
	"flame30",
	"flame31",
	"flame32",
	"flame33",
	"flame34",
	"flame35",
	"flame36",
	"flame38",
	"flame39",
	"flame40",
	"flame41",
	"flame42",
	"flame43",
	"flame44",
	"flame45"
};

CParticle gParticleArray[MAX_PARTICLES_ON_SCREEN];

RwTexture *gpSmokeTex[MAX_SMOKE_FILES];
RwTexture *gpSmoke2Tex[MAX_SMOKE2_FILES];
RwTexture *gpRubberTex[MAX_RUBBER_FILES];
RwTexture *gpRainSplashTex[MAX_RAINSPLASH_FILES];
RwTexture *gpWatersprayTex[MAX_WATERSPRAY_FILES];
RwTexture *gpExplosionMediumTex[MAX_EXPLOSIONMEDIUM_FILES];
RwTexture *gpGunFlashTex[MAX_GUNFLASH_FILES];
RwTexture *gpRainSplashupTex[MAX_RAINSPLASHUP_FILES];
RwTexture *gpBirdfrontTex[MAX_BIRDFRONT_FILES];
RwTexture *gpCarDebrisTex[MAX_CARDEBRIS_FILES];
RwTexture *gpCarSplashTex[MAX_CARSPLASH_FILES];

RwTexture *gpFlame1Tex;
RwTexture *gpFlame5Tex;
RwTexture *gpRainDropSmallTex;
RwTexture *gpBloodTex;
RwTexture *gpLeafTex;
RwTexture *gpCloudTex1;
RwTexture *gpCloudTex4;
RwTexture *gpBloodSmallTex;
RwTexture *gpGungeTex;
RwTexture *gpCollisionSmokeTex;
RwTexture *gpBulletHitTex;
RwTexture *gpGunShellTex;
RwTexture *gpWakeOldTex;
RwTexture *gpPointlightTex;

RwRaster  *gpSmokeRaster[MAX_SMOKE_FILES];
RwRaster  *gpSmoke2Raster[MAX_SMOKE2_FILES];
RwRaster  *gpRubberRaster[MAX_RUBBER_FILES];
RwRaster  *gpRainSplashRaster[MAX_RAINSPLASH_FILES];
RwRaster  *gpWatersprayRaster[MAX_WATERSPRAY_FILES];
RwRaster  *gpExplosionMediumRaster[MAX_EXPLOSIONMEDIUM_FILES];
RwRaster  *gpGunFlashRaster[MAX_GUNFLASH_FILES];
RwRaster  *gpRainSplashupRaster[MAX_RAINSPLASHUP_FILES];
RwRaster  *gpBirdfrontRaster[MAX_BIRDFRONT_FILES];
RwRaster  *gpCarDebrisRaster[MAX_CARDEBRIS_FILES];
RwRaster  *gpCarSplashRaster[MAX_CARSPLASH_FILES];

RwRaster  *gpFlame1Raster;
RwRaster  *gpFlame5Raster;
RwRaster  *gpRainDropSmallRaster;
RwRaster  *gpBloodRaster;
RwRaster  *gpLeafRaster;
RwRaster  *gpCloudRaster1;
RwRaster  *gpCloudRaster4;
RwRaster  *gpBloodSmallRaster;
RwRaster  *gpGungeRaster;
RwRaster  *gpCollisionSmokeRaster;
RwRaster  *gpBulletHitRaster;
RwRaster  *gpGunShellRaster;
RwRaster  *gpWakeOldRaster;
RwRaster  *gpPointlightRaster;

RwTexture *gpRainDropTex[MAX_RAINDROP_FILES];
RwRaster  *gpRainDropRaster[MAX_RAINDROP_FILES];

RwTexture *gpCarFlameTex[MAX_CARFLAME_FILES];
RwRaster  *gpCarFlameRaster[MAX_CARFLAME_FILES];

float      ParticleEngine::ms_afRandTable[ParticleEngine::RAND_TABLE_SIZE];
CParticle *ParticleEngine::m_pUnusedListHead;
float      ParticleEngine::m_SinTable[ParticleEngine::SIN_COS_TABLE_SIZE];
float      ParticleEngine::m_CosTable[ParticleEngine::SIN_COS_TABLE_SIZE];



int32 Randomizer;

int32 nParticleCreationInterval = 1;
float fParticleScaleLimit       = 0.5f;


void ParticleEngine::InitParticleArray(CParticle *array, uint32 size)
{
	m_pUnusedListHead = array;

	for ( uint32 i = 0; i < size; i++ )
	{
		if ( i == size - 1 )
			array[i].m_pNext = NULL;
		else
			array[i].m_pNext = &array[i + 1];

		array[i].m_vecPosition = CVector(0.0f, 0.0f, 0.0f);

		array[i].m_vecVelocity = CVector(0.0f, 0.0f, 0.0f);

		array[i].m_nTimeWhenWillBeDestroyed = 0;

		array[i].m_nTimeWhenColorWillBeChanged = 0;

		array[i].m_fSize = 0.2f;

		array[i].m_fExpansionRate = 0.0f;

		array[i].m_nColorIntensity = 255;

		array[i].m_nFadeToBlackTimer = 0;

		array[i].m_nAlpha = 255;

		array[i].m_nFadeAlphaTimer = 0;

		array[i].m_nCurrentZRotation = 0;

		array[i].m_nZRotationTimer = 0;

		array[i].m_fCurrentZRadius = 0.0f;

		array[i].m_nZRadiusTimer = 0;

		array[i].m_nCurrentFrame = 0;

		array[i].m_nAnimationSpeedTimer = 0;

		array[i].m_nRotation = 0;

		array[i].m_nRotationStep = 0;
	}
}

static float updateAccumulator = 0.0f;

void ParticleEngine::ReloadConfig()
{
	Kprintf("Initialising CParticleMgr...");

	mod_ParticleSystemManager.Initialise();

	Kprintf("Initialising CParticle...");

	InitParticleArray(gParticleArray, ARRAY_SIZE(gParticleArray));
	updateAccumulator = 0.0f;

}

void ParticleEngine::FixFlame5Bug(bool enable)
{
	if ( gpFlame5Tex == NULL || gpFlame1Tex == NULL )
		return;

	if ( enable )
		gpFlame5Raster = RwTextureGetRaster(gpFlame5Tex);
	else
		gpFlame5Raster = RwTextureGetRaster(gpFlame1Tex);
}


bool ParticleEngine::LoadResources()
{
	if (!mod_ParticleSystemManager.LoadParticleData()) return false;
	const int32 slot = CTxdStore::AddTxdSlot("particleREXBOX");
	_rwD3D8TexDictionaryEnableRasterFormatConversion(true);
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "ParticleEx/XBOX/particle.txd");
	const bool loaded = stream && CTxdStore::LoadTxd(slot, stream);
	if (stream) RwStreamClose(stream, nil);
	if (!loaded) {
		CTxdStore::RemoveTxdSlot(slot);
		return false;
	}
	const char *required[] = {"flame1", "flame5", "rainsmall", "blood", "gameleaf01_64", "cloud3", "cloudmasked", "bloodsplat2", "gunge", "collisionsmoke", "bullethitsmoke", "gunshell", "wake_old", "pointlight", "smoke1", "smoke2", "smoke3", "smoke4", "smoke5", "smokeII_1", "smokeII_2", "smokeII_3", "smokeII_4", "smokeII_5", "rubber1", "rubber2", "rubber3", "rubber4", "rubber5", "splash1", "splash2", "splash3", "splash4", "splash5", "waterspray1", "waterspray2", "waterspray3", "explo01", "explo02", "explo03", "explo04", "explo05", "explo06", "explo07", "explo08", "explo09", "explo10", "explo11", "explo12", "explo13", "explo14", "explo15", "gunflash1", "gunflash2", "gunflash3", "gunflash4", "raindrop1", "raindrop2", "raindrop3", "raindrop4", "splash_up1", "splash_up2", "birdf_01", "birdf_02", "birdf_03", "birdf_04", "cardebris_01", "cardebris_02", "cardebris_03", "cardebris_04", "carsplash_01", "carsplash_02", "carsplash_03", "carsplash_04", "flame01", "flame02", "flame03", "flame04", "flame05", "flame06", "flame07", "flame08", "flame09", "flame10", "flame11", "flame12", "flame13", "flame14", "flame15", "flame16", "flame17", "flame18", "flame19", "flame20", "flame21", "flame22", "flame23", "flame24", "flame25", "flame26", "flame27", "flame28", "flame29", "flame30", "flame31", "flame32", "flame33", "flame34", "flame35", "flame36", "flame38", "flame39", "flame40", "flame41", "flame42", "flame43", "flame44", "flame45"};
	for (const char *name : required) {
		RwTexture *texture = RwTexDictionaryFindNamedTexture(CTxdStore::GetSlot(slot)->texDict, name);
		if (!texture || !RwTextureGetRaster(texture)) {
			CTxdStore::RemoveTxdSlot(slot);
			return false;
		}
	}
	CTxdStore::AddRef(slot);
	return true;
}

void ParticleEngine::Initialise()
{
	ReloadConfig();

#if INJECT_PARTICLE == FALSE
	CParticleObject::Initialise();
#endif

	float randVal = -1.0f;
	for ( int32 i = 0; i < RAND_TABLE_SIZE; i++ )
	{
		ms_afRandTable[i] = randVal;
		randVal += 0.1f;
	}

	for ( int32 i = 0; i < SIN_COS_TABLE_SIZE; i++ )
	{
		float angle = DEGTORAD(float(i) * float(360.0f / SIN_COS_TABLE_SIZE));

		m_SinTable[i] = sin(angle);
		m_CosTable[i] = cos(angle);
	}

int32 slot = CTxdStore::FindTxdSlot("particleREXBOX");

	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(slot);

	for ( int32 i = 0; i < MAX_SMOKE_FILES; i++ )
	{
		gpSmokeTex[i] = RwTextureRead(SmokeFiles[i], NULL);
		gpSmokeRaster[i] = RwTextureGetRaster(gpSmokeTex[i]);
	}

	for ( int32 i = 0; i < MAX_SMOKE2_FILES; i++ )
	{
		gpSmoke2Tex[i] = RwTextureRead(Smoke2Files[i], NULL);
		gpSmoke2Raster[i] = RwTextureGetRaster(gpSmoke2Tex[i]);
	}

	for ( int32 i = 0; i < MAX_RUBBER_FILES; i++ )
	{
		gpRubberTex[i] = RwTextureRead(RubberFiles[i], NULL);
		gpRubberRaster[i] = RwTextureGetRaster(gpRubberTex[i]);
	}

	for ( int32 i = 0; i < MAX_RAINSPLASH_FILES; i++ )
	{
		gpRainSplashTex[i] = RwTextureRead(RainSplashFiles[i], NULL);
		gpRainSplashRaster[i] = RwTextureGetRaster(gpRainSplashTex[i]);
	}

	for ( int32 i = 0; i < MAX_WATERSPRAY_FILES; i++ )
	{
		gpWatersprayTex[i] = RwTextureRead(WatersprayFiles[i], NULL);
		gpWatersprayRaster[i] = RwTextureGetRaster(gpWatersprayTex[i]);
	}

	for ( int32 i = 0; i < MAX_EXPLOSIONMEDIUM_FILES; i++ )
	{
		gpExplosionMediumTex[i] = RwTextureRead(ExplosionMediumFiles[i], NULL);
		gpExplosionMediumRaster[i] = RwTextureGetRaster(gpExplosionMediumTex[i]);
	}

	for ( int32 i = 0; i < MAX_GUNFLASH_FILES; i++ )
	{
		gpGunFlashTex[i] = RwTextureRead(GunFlashFiles[i], NULL);
		gpGunFlashRaster[i] = RwTextureGetRaster(gpGunFlashTex[i]);
	}

	for ( int32 i = 0; i < MAX_RAINDROP_FILES; i++ )
	{
		gpRainDropTex[i] = RwTextureRead(RaindropFiles[i], NULL);
		gpRainDropRaster[i] = RwTextureGetRaster(gpRainDropTex[i]);
	}

	for ( int32 i = 0; i < MAX_RAINSPLASHUP_FILES; i++ )
	{
		gpRainSplashupTex[i] = RwTextureRead(RainSplashupFiles[i], NULL);
		gpRainSplashupRaster[i] = RwTextureGetRaster(gpRainSplashupTex[i]);
	}

	for ( int32 i = 0; i < MAX_BIRDFRONT_FILES; i++ )
	{
		gpBirdfrontTex[i] = RwTextureRead(BirdfrontFiles[i], NULL);
		gpBirdfrontRaster[i] = RwTextureGetRaster(gpBirdfrontTex[i]);
	}

	for ( int32 i = 0; i < MAX_CARDEBRIS_FILES; i++ )
	{
		gpCarDebrisTex[i] = RwTextureRead(CardebrisFiles[i], NULL);
		gpCarDebrisRaster[i] = RwTextureGetRaster(gpCarDebrisTex[i]);
	}

	for ( int32 i = 0; i < MAX_CARSPLASH_FILES; i++ )
	{
		gpCarSplashTex[i] = RwTextureRead(CarsplashFiles[i], NULL);
		gpCarSplashRaster[i] = RwTextureGetRaster(gpCarSplashTex[i]);
	}

	for ( int32 i = 0; i < MAX_CARFLAME_FILES; i++ )
	{
		gpCarFlameTex[i] = RwTextureRead(CarFlameFiles[i], NULL);

		if ( gpCarFlameTex[i] != NULL )
			gpCarFlameRaster[i] = RwTextureGetRaster(gpCarFlameTex[i]);
	}

	gpFlame1Tex = RwTextureRead("flame1", NULL);
	gpFlame1Raster = RwTextureGetRaster(gpFlame1Tex);

	gpFlame5Tex = RwTextureRead("flame5", NULL);

#if FIX_FLAME5_BUG == FALSE
	gpFlame5Raster = RwTextureGetRaster(gpFlame1Tex);	// copy-paste bug ?
#else
	gpFlame5Raster = RwTextureGetRaster(gpFlame5Tex);
#endif

	gpRainDropSmallTex = RwTextureRead("rainsmall", NULL);
	gpRainDropSmallRaster = RwTextureGetRaster(gpRainDropSmallTex);

	gpBloodTex = RwTextureRead("blood", NULL);
	gpBloodRaster = RwTextureGetRaster(gpBloodTex);

	gpLeafTex = RwTextureRead("gameleaf01_64", NULL);
	gpLeafRaster = RwTextureGetRaster(gpLeafTex);

	gpCloudTex1 = RwTextureRead("cloud3", NULL);
	gpCloudRaster1 = RwTextureGetRaster(gpCloudTex1);

	gpCloudTex4 = RwTextureRead("cloudmasked", NULL);
	gpCloudRaster4 = RwTextureGetRaster(gpCloudTex4);

	gpBloodSmallTex = RwTextureRead("bloodsplat2", NULL);
	gpBloodSmallRaster = RwTextureGetRaster(gpBloodSmallTex);

	gpGungeTex = RwTextureRead("gunge", NULL);
	gpGungeRaster = RwTextureGetRaster(gpGungeTex);

	gpCollisionSmokeTex = RwTextureRead("collisionsmoke", NULL);
	gpCollisionSmokeRaster = RwTextureGetRaster(gpCollisionSmokeTex);

	gpBulletHitTex = RwTextureRead("bullethitsmoke", NULL);
	gpBulletHitRaster = RwTextureGetRaster(gpBulletHitTex);

	gpGunShellTex = RwTextureRead("gunshell", NULL);
	gpGunShellRaster = RwTextureGetRaster(gpGunShellTex);

	gpWakeOldTex = RwTextureRead("wake_old", NULL);
	gpWakeOldRaster = RwTextureGetRaster(gpWakeOldTex);

	gpPointlightTex = RwTextureRead("pointlight", NULL);
	gpPointlightRaster = RwTextureGetRaster(gpPointlightTex);

	CTxdStore::PopCurrentTxd();

	for ( int32 i = 0; i < MAX_PARTICLES; i++ )
	{
		tParticleSystemData *entry = &mod_ParticleSystemManager.m_aParticles[i];

		switch ( i )
		{
			case PARTICLE_BLOOD:
				entry->m_ppRaster = &gpBloodRaster;
				break;

			case PARTICLE_BLOOD_SMALL:
			case PARTICLE_BLOOD_SPURT:
				entry->m_ppRaster = &gpBloodSmallRaster;
				break;

			case PARTICLE_DEBRIS2:
				entry->m_ppRaster = &gpGungeRaster;
				break;

			case PARTICLE_GUNFLASH:
			case PARTICLE_GUNFLASH_NOANIM:
				entry->m_ppRaster = gpGunFlashRaster;
				break;

			case PARTICLE_GUNSMOKE:
			case PARTICLE_SPLASH:
				entry->m_ppRaster = NULL;
				break;

			case PARTICLE_FLAME:
			case PARTICLE_CARFLAME:
			case PARTICLE_CARFLAME_MOVING:
				if ( gpCarFlameRaster[0] != NULL )
					entry->m_ppRaster = gpCarFlameRaster;
				else
					entry->m_ppRaster = &gpFlame1Raster;
				break;
			case PARTICLE_FIREBALL:
				if ( gpCarFlameRaster[0] != NULL )
					entry->m_ppRaster = gpCarFlameRaster;
				else
					entry->m_ppRaster = &gpFlame5Raster;
				break;

			case PARTICLE_RAIN_SPLASH:
			case PARTICLE_RAIN_SPLASH_BIGGROW:
				entry->m_ppRaster = gpRainSplashRaster;
				break;

			case PARTICLE_RAIN_SPLASHUP:
				entry->m_ppRaster = gpRainSplashupRaster;
				break;

			case PARTICLE_WATERSPRAY:
				entry->m_ppRaster = gpWatersprayRaster;
				break;

			case PARTICLE_SHARD:
			case PARTICLE_RAINDROP:
			case PARTICLE_RAINDROP_2D:
				entry->m_ppRaster = gpRainDropRaster;
				break;

			case PARTICLE_EXPLOSION_MEDIUM:
			case PARTICLE_EXPLOSION_LARGE:
			case PARTICLE_EXPLOSION_MFAST:
			case PARTICLE_EXPLOSION_LFAST:
				entry->m_ppRaster = gpExplosionMediumRaster;
				break;

			case PARTICLE_BOAT_WAKE:
				entry->m_ppRaster = &gpWakeOldRaster;
				break;

			case PARTICLE_CAR_SPLASH:
			case PARTICLE_WATER_HYDRANT:
			case PARTICLE_PED_SPLASH:
				entry->m_ppRaster = gpCarSplashRaster;
				break;

			case PARTICLE_SPARK:
			case PARTICLE_SPARK_SMALL:
			case PARTICLE_RAINDROP_SMALL:
			case PARTICLE_HELI_ATTACK:
				entry->m_ppRaster = &gpRainDropSmallRaster;
				break;

			case PARTICLE_DEBRIS:
			case PARTICLE_TREE_LEAVES:
				entry->m_ppRaster = &gpLeafRaster;
				break;

			case PARTICLE_CAR_DEBRIS:
			case PARTICLE_HELI_DEBRIS:
				entry->m_ppRaster = gpCarDebrisRaster;
				break;

			case PARTICLE_WHEEL_DIRT:
			case PARTICLE_STEAM2:
			case PARTICLE_STEAM_NY:
			case PARTICLE_STEAM_NY_SLOWMOTION:
			case PARTICLE_ENGINE_STEAM:
			case PARTICLE_BOAT_THRUSTJET:
			case PARTICLE_PEDFOOT_DUST:
			case PARTICLE_EXHAUST_FUMES:
				entry->m_ppRaster = gpSmoke2Raster;
				break;

			case PARTICLE_GUNSMOKE2:
			case PARTICLE_RUBBER_SMOKE:
				entry->m_ppRaster = gpRubberRaster;
				break;

			case PARTICLE_CARCOLLISION_DUST:
			case PARTICLE_BURNINGRUBBER_SMOKE:
				entry->m_ppRaster = &gpCollisionSmokeRaster;
				break;

			case PARTICLE_WHEEL_WATER:
			case PARTICLE_WATER:
			case PARTICLE_SMOKE:
			case PARTICLE_SMOKE_SLOWMOTION:
			case PARTICLE_GARAGEPAINT_SPRAY:
			case PARTICLE_STEAM:
			case PARTICLE_BOAT_SPLASH:
			case PARTICLE_WATER_CANNON:
			case PARTICLE_EXTINGUISH_STEAM:
			case PARTICLE_HELI_DUST:
			case PARTICLE_PAINT_SMOKE:
			case PARTICLE_BULLETHIT_SMOKE:
				entry->m_ppRaster = gpSmokeRaster;
				break;

			case PARTICLE_GUNSHELL_FIRST:
			case PARTICLE_GUNSHELL:
			case PARTICLE_GUNSHELL_BUMP1:
			case PARTICLE_GUNSHELL_BUMP2:
				entry->m_ppRaster = &gpGunShellRaster;
				break;

			case PARTICLE_ENGINE_SMOKE:
			case PARTICLE_ENGINE_SMOKE2:
			case PARTICLE_CARFLAME_SMOKE:
			case PARTICLE_FIREBALL_SMOKE:
			case PARTICLE_TEST:
				entry->m_ppRaster = &gpCloudRaster4;
				break;

			case PARTICLE_BIRD_FRONT:
				entry->m_ppRaster = gpBirdfrontRaster;
				break;
		}
	}

	Kprintf("CParticle ready");
}

void ParticleEngine::Shutdown()
{
	Kprintf("Shutting down CParticle...");

	for ( int32 i = 0; i < MAX_SMOKE_FILES; i++ )
	{
		RwTextureDestroy(gpSmokeTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpSmokeTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_SMOKE2_FILES; i++ )
	{
		RwTextureDestroy(gpSmoke2Tex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpSmoke2Tex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_RUBBER_FILES; i++ )
	{
		RwTextureDestroy(gpRubberTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpRubberTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_RAINSPLASH_FILES; i++ )
	{
		RwTextureDestroy(gpRainSplashTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpRainSplashTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_WATERSPRAY_FILES; i++ )
	{
		RwTextureDestroy(gpWatersprayTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpWatersprayTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_EXPLOSIONMEDIUM_FILES; i++ )
	{
		RwTextureDestroy(gpExplosionMediumTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpExplosionMediumTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_GUNFLASH_FILES; i++ )
	{
		RwTextureDestroy(gpGunFlashTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpGunFlashTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_RAINDROP_FILES; i++ )
	{
		RwTextureDestroy(gpRainDropTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpRainDropTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_RAINSPLASHUP_FILES; i++ )
	{
		RwTextureDestroy(gpRainSplashupTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpRainSplashupTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_BIRDFRONT_FILES; i++ )
	{
		RwTextureDestroy(gpBirdfrontTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpBirdfrontTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_CARDEBRIS_FILES; i++ )
	{
		RwTextureDestroy(gpCarDebrisTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpCarDebrisTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_CARSPLASH_FILES; i++ )
	{
		RwTextureDestroy(gpCarSplashTex[i]);
#if GTA3_1_1_PATCH == TRUE
		gpCarSplashTex[i] = NULL;
#endif
	}

	for ( int32 i = 0; i < MAX_CARFLAME_FILES; i++ )
	{
		if ( gpCarSplashTex[i] )
		{
			RwTextureDestroy(gpCarFlameTex[i]);
			gpCarFlameTex[i] = NULL;
		}
	}


	RwTextureDestroy(gpFlame1Tex);
#if GTA3_1_1_PATCH == TRUE
	gpFlame1Tex = NULL;
#endif

	RwTextureDestroy(gpFlame5Tex);
#if GTA3_1_1_PATCH == TRUE
	gpFlame5Tex = NULL;
#endif

	RwTextureDestroy(gpRainDropSmallTex);
#if GTA3_1_1_PATCH == TRUE
	gpRainDropSmallTex = NULL;
#endif

	RwTextureDestroy(gpBloodTex);
#if GTA3_1_1_PATCH == TRUE
	gpBloodTex = NULL;
#endif

	RwTextureDestroy(gpLeafTex);
#if GTA3_1_1_PATCH == TRUE
	gpLeafTex = NULL;
#endif

	RwTextureDestroy(gpCloudTex1);
#if GTA3_1_1_PATCH == TRUE
	gpCloudTex1 = NULL;
#endif

	RwTextureDestroy(gpCloudTex4);
#if GTA3_1_1_PATCH == TRUE
	gpCloudTex4 = NULL;
#endif

	RwTextureDestroy(gpBloodSmallTex);
#if GTA3_1_1_PATCH == TRUE
	gpBloodSmallTex = NULL;
#endif

	RwTextureDestroy(gpGungeTex);
#if GTA3_1_1_PATCH == TRUE
	gpGungeTex = NULL;
#endif

	RwTextureDestroy(gpCollisionSmokeTex);
#if GTA3_1_1_PATCH == TRUE
	gpCollisionSmokeTex = NULL;
#endif

	RwTextureDestroy(gpBulletHitTex);
#if GTA3_1_1_PATCH == TRUE
	gpBulletHitTex = NULL;
#endif

	RwTextureDestroy(gpGunShellTex);
#if GTA3_1_1_PATCH == TRUE
	gpGunShellTex = NULL;
#endif

	RwTextureDestroy(gpWakeOldTex);
#if GTA3_1_1_PATCH == TRUE
	gpWakeOldTex = NULL;
#endif

	RwTextureDestroy(gpPointlightTex);
#if GTA3_1_1_PATCH == TRUE
	gpPointlightTex = NULL;
#endif

	int32 slot;
#if INJECT_PARTICLE == FALSE
	slot = CTxdStore::FindTxdSlot("particle");
	CTxdStore::RemoveTxdSlot(slot);
#endif

#if USE_CUSTOM_DIR == TRUE
	slot = CTxdStore::FindTxdSlot("particleREXBOX");
	CTxdStore::RemoveTxdSlot(slot);
#endif

	Kprintf("CParticle shut down");
}

CParticle *ParticleEngine::AddParticle(tParticleType type, CVector const &vecPos, CVector const &vecDir, CEntity *pEntity, float fSize, int32 nRotationSpeed, int32 nRotation, int32 nCurFrame, int32 nLifeSpan)
{
	CRGBA color(0, 0, 0, 0);
	return AddParticle(type, vecPos, vecDir, pEntity, fSize, color, nRotationSpeed, nRotation, nCurFrame, nLifeSpan);
}

CParticle *ParticleEngine::AddParticle(tParticleType type, CVector const &vecPos, CVector const &vecDir, CEntity *pEntity, float fSize, RwRGBA const &color, int32 nRotationSpeed, int32 nRotation, int32 nCurFrame, int32 nLifeSpan)
{
	if (type < PARTICLE_FIRST || type > PARTICLE_LAST) return nil;
	if ( CTimer::GetIsUserPaused() || CTimer::GetIsCodePaused() )
		return NULL;

	CParticle *pParticle = m_pUnusedListHead;

	if ( pParticle == NULL )
		return NULL;

	tParticleSystemData *psystem = &mod_ParticleSystemManager.m_aParticles[type];

	if ( psystem->m_fCreateRange != 0.0f && psystem->m_fCreateRange < ( TheCamera.GetPosition() - vecPos ).MagnitudeSqr() )
		return NULL;


	pParticle->m_fSize = psystem->m_fDefaultInitialRadius;
	pParticle->m_fExpansionRate = psystem->m_fExpansionRate;

	if ( nLifeSpan != 0 )
		pParticle->m_nTimeWhenWillBeDestroyed = CTimer::GetTimeInMilliseconds() + nLifeSpan;
	else
		pParticle->m_nTimeWhenWillBeDestroyed = CTimer::GetTimeInMilliseconds() + psystem->m_nLifeSpan;

	pParticle->m_nColorIntensity = psystem->m_nFadeToBlackInitialIntensity;
	pParticle->m_nAlpha = psystem->m_nFadeAlphaInitialIntensity;
	pParticle->m_nCurrentZRotation = psystem->m_nZRotationInitialAngle;
	pParticle->m_fCurrentZRadius = psystem->m_fInitialZRadius;

	if ( nCurFrame != 0 )
		pParticle->m_nCurrentFrame = nCurFrame;
	else
		pParticle->m_nCurrentFrame = psystem->m_nStartAnimationFrame;

	pParticle->m_nFadeToBlackTimer = 0;
	pParticle->m_nFadeAlphaTimer = 0;
	pParticle->m_nZRotationTimer = 0;
	pParticle->m_nZRadiusTimer = 0;
	pParticle->m_nAnimationSpeedTimer = 0;
	pParticle->m_fZGround = 0.0f;
	pParticle->m_vecPosition = vecPos;
	pParticle->m_vecVelocity = vecDir;
	pParticle->m_vecParticleMovementOffset = CVector(0.0f, 0.0f, 0.0f);
	pParticle->m_nTimeWhenColorWillBeChanged = 0;

	if ( color.alpha != 0 )
		RwRGBAAssign(&pParticle->m_Color, &color);
	else
	{
		RwRGBAAssign(&pParticle->m_Color, &psystem->m_RenderColouring);

		if ( psystem->m_ColorFadeTime != 0 )
			pParticle->m_nTimeWhenColorWillBeChanged = CTimer::GetTimeInMilliseconds() + psystem->m_ColorFadeTime;

		if ( psystem->m_InitialColorVariation != 0 )
		{
			int32 ColorVariation = CGeneral::GetRandomNumberInRange(-psystem->m_InitialColorVariation, psystem->m_InitialColorVariation);
			//float ColorVariation = CGeneral::GetRandomNumberInRange((float)-psystem->m_InitialColorVariation, (float)psystem->m_InitialColorVariation);

			pParticle->m_Color.red   = clamp(pParticle->m_Color.red +
				PERCENT(pParticle->m_Color.red, ColorVariation),
				0, 255);

			pParticle->m_Color.green = clamp(pParticle->m_Color.green +
				PERCENT(pParticle->m_Color.green, ColorVariation),
				0, 255);

			pParticle->m_Color.blue  = clamp(pParticle->m_Color.blue +
				PERCENT(pParticle->m_Color.blue, ColorVariation),
				0, 255);
		}
	}

	pParticle->m_nRotation = nRotation;

	if ( pParticle->m_nRotation >= 360 )
		pParticle->m_nRotation -= 360;
	else if ( pParticle->m_nRotation < 0 )
		pParticle->m_nRotation += 360;

	if ( nRotationSpeed != 0 )
		pParticle->m_nRotationStep = nRotationSpeed;
	else
		pParticle->m_nRotationStep = psystem->m_nRotationSpeed;

	if ( CGeneral::GetRandomNumber() & 1 )
		pParticle->m_nRotationStep = -pParticle->m_nRotationStep;

	pParticle->m_vecScreenPosition.x = 0.0f; // bug ?

	if ( psystem->m_fPositionRandomError != 0.0f )
	{
		pParticle->m_vecPosition.x += psystem->m_fPositionRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];
		pParticle->m_vecPosition.y += psystem->m_fPositionRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];

		if ( psystem->Flags & RAND_VERT_V )
			pParticle->m_vecPosition.z += psystem->m_fPositionRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];
	}

	if ( psystem->m_fVelocityRandomError != 0.0f )
	{
		pParticle->m_vecVelocity.x += psystem->m_fVelocityRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];
		pParticle->m_vecVelocity.y += psystem->m_fVelocityRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];

		if ( psystem->Flags & RAND_VERT_V )
			pParticle->m_vecVelocity.z += psystem->m_fVelocityRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];
	}

	if ( psystem->m_fExpansionRateError != 0.0f )
		pParticle->m_fExpansionRate += psystem->m_fExpansionRateError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE] + psystem->m_fExpansionRateError;

	if ( psystem->m_nRotationRateError != 0 )
		pParticle->m_nRotationStep += CGeneral::GetRandomNumberInRange(-psystem->m_nRotationRateError, psystem->m_nRotationRateError);

	if ( psystem->m_nLifeSpanErrorShape != 0 )
	{
		float randVal = ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];
		if ( randVal > 0.0f )
			pParticle->m_nTimeWhenWillBeDestroyed += int32(float(psystem->m_nLifeSpan) * randVal * float(psystem->m_nLifeSpanErrorShape));
		else
			pParticle->m_nTimeWhenWillBeDestroyed += int32(float(psystem->m_nLifeSpan) * randVal / float(psystem->m_nLifeSpanErrorShape));
	}

	if ( psystem->Flags & ZCHECK_FIRST )
	{
		static bool bValidGroundFound = false;
		static CVector LastTestCoors;
		static float LastTestGroundZ;

		if ( bValidGroundFound
			&& vecPos.x == LastTestCoors.x
			&& vecPos.y == LastTestCoors.y
			&& vecPos.z == LastTestCoors.z )
		{
			pParticle->m_fZGround = LastTestGroundZ;
		}
		else
		{
			bValidGroundFound = false;

			CColPoint point;
			CEntity *entity;

			if ( !CWorld::ProcessVerticalLine(
						pParticle->m_vecPosition + CVector(0.0f, 0.0f, 0.5f),
						-100.0f, point, entity, true, true, false, false, true, false, NULL) )
			{
				return NULL;
			}

			if ( point.point.z >= pParticle->m_vecPosition.z )
				return NULL;

			pParticle->m_fZGround = point.point.z;
			bValidGroundFound = true;
			LastTestCoors = vecPos;
			LastTestGroundZ = point.point.z;
		}
	}

	if ( psystem->Flags & ZCHECK_BUMP )
	{
		static float Z_Ground = 0.0f;

		if ( psystem->Flags & ZCHECK_BUMP_FIRST )
		{
			bool bZFound = false;

			Z_Ground = CWorld::FindGroundZFor3DCoord(vecPos.x, vecPos.y, vecPos.z, (bool *)&bZFound);

			if ( bZFound == false )
				return NULL;

			pParticle->m_fZGround = Z_Ground;
		}

		pParticle->m_fZGround = Z_Ground;
	}

	switch ( type )
	{
		case PARTICLE_DEBRIS:
			pParticle->m_vecVelocity.z *= CGeneral::GetRandomNumberInRange(0.5f, 3.0f);
			break;

		case PARTICLE_EXPLOSION_MEDIUM:
			pParticle->m_nColorIntensity -= 30 * (CGeneral::GetRandomNumber() & 1); // mb "+= -30 * rand" here ?
			pParticle->m_nAnimationSpeedTimer = CGeneral::GetRandomNumber() & 7;
			pParticle->m_fSize = CGeneral::GetRandomNumberInRange(0.3f, 0.8f);
			pParticle->m_vecPosition.z -= CGeneral::GetRandomNumberInRange(-0.1f, 0.1f);
			break;

		case PARTICLE_EXPLOSION_LARGE:
			pParticle->m_nColorIntensity -= 30 * (CGeneral::GetRandomNumber() & 1); // mb "+= -30 * rand" here ?
			pParticle->m_nAnimationSpeedTimer = CGeneral::GetRandomNumber() & 7;
			pParticle->m_fSize = CGeneral::GetRandomNumberInRange(0.8f, 1.4f);
			pParticle->m_vecPosition.z -= CGeneral::GetRandomNumberInRange(-0.3f, 0.3f);
			break;

		case PARTICLE_WATER_HYDRANT:
			pParticle->m_vecPosition.z += 20.0f * psystem->m_fPositionRandomError * ms_afRandTable[(uint16)CGeneral::GetRandomNumber() % RAND_TABLE_SIZE];
			break;
	}

	if ( fSize != 0.0f )
		pParticle->m_fSize = fSize;

	m_pUnusedListHead = pParticle->m_pNext;

	pParticle->m_pNext = psystem->m_pParticles;

	psystem->m_pParticles = pParticle;

	return pParticle;
}

// Fade, animation and rotation counters in the console code are frame-based.
// Tick them at 30 Hz; movement uses the corresponding simulation timestep.
void ParticleEngine::Update()
{
	if (CTimer::GetIsPaused()) return;
	const float tick = 50.0f / 30.0f;
	updateAccumulator += Min(CTimer::GetTimeStep(), 10.0f);
	while (updateAccumulator >= tick) {
		updateAccumulator -= tick;
		UpdateStep(tick);
	}
}

void ParticleEngine::UpdateStep(float timeStep)
{
	if ( CTimer::GetIsUserPaused() || CTimer::GetIsCodePaused() )
		return;

	CRGBA color(0, 0, 0, 0);

	float fFricDeccel50 = pow(0.50f, timeStep);
	float fFricDeccel80 = pow(0.80f, timeStep);
	float fFricDeccel90 = pow(0.90f, timeStep);
	float fFricDeccel95 = pow(0.95f, timeStep);
	float fFricDeccel96 = pow(0.96f, timeStep);
	float fFricDeccel99 = pow(0.99f, timeStep);

	// PS2 owns emitter updates in hybrid mode; never emit twice per tick.
	if (ParticleEx::ActiveSystem != ParticleEx::PS2Xbox) CParticleObject::UpdateAll();

	for ( int32 i = 0; i < MAX_PARTICLES; i++ )
	{
		tParticleSystemData *psystem = &mod_ParticleSystemManager.m_aParticles[i];
		CParticle *particle = psystem->m_pParticles;
		CParticle *prevParticle = NULL;
		bool bRemoveParticle;

		if ( particle == NULL )
			continue;

		for ( ; particle != NULL; _Next(particle, prevParticle, psystem, bRemoveParticle) )
		{
			bRemoveParticle = false;

			CVector moveStep = particle->m_vecPosition + ( particle->m_vecVelocity * timeStep );

			if (  CTimer::GetTimeInMilliseconds() > particle->m_nTimeWhenWillBeDestroyed || particle->m_nAlpha == 0 )
			{
				bRemoveParticle = true;
				continue;
			}

			if ( particle->m_nTimeWhenColorWillBeChanged != 0 )
			{
				if ( particle->m_nTimeWhenColorWillBeChanged > CTimer::GetTimeInMilliseconds() )
				{
					float colorMul = 1.0f - float(particle->m_nTimeWhenColorWillBeChanged - CTimer::GetTimeInMilliseconds()) / float(psystem->m_ColorFadeTime);

					particle->m_Color.red = clamp(
						psystem->m_RenderColouring.red + int32(float(psystem->m_FadeDestinationColor.red - psystem->m_RenderColouring.red) * colorMul),
						0, 255);

					particle->m_Color.green = clamp(
						psystem->m_RenderColouring.green + int32(float(psystem->m_FadeDestinationColor.green - psystem->m_RenderColouring.green) * colorMul),
						0, 255);

					particle->m_Color.blue = clamp(
						psystem->m_RenderColouring.blue + int32(float(psystem->m_FadeDestinationColor.blue - psystem->m_RenderColouring.blue) * colorMul),
						0, 255);
				}
				else
					RwRGBAAssign(&particle->m_Color, &psystem->m_FadeDestinationColor);
			}

			if ( psystem->Flags & CLIPOUT2D )
			{
				if ( particle->m_vecPosition.x < -10.0f || particle->m_vecPosition.x > float(SCREEN_WIDTH) + 10.0f
					|| particle->m_vecPosition.y < -10.0f || particle->m_vecPosition.y > float(SCREEN_HEIGHT) + 10.0f )
				{
					bRemoveParticle = true;
					continue;
				}
			}

			float size = particle->m_fSize + particle->m_fExpansionRate;

			if ( size < 0.0f )
			{
				bRemoveParticle = true;
				continue;
			}

			particle->m_fSize = size;

			switch ( psystem->m_nFrictionDecceleration )
			{
				case 50:
					particle->m_vecVelocity *= fFricDeccel50;
					break;

				case 80:
					particle->m_vecVelocity *= fFricDeccel80;
					break;

				case 90:
					particle->m_vecVelocity *= fFricDeccel90;
					break;

				case 95:
					particle->m_vecVelocity *= fFricDeccel95;
					break;

				case 96:
					particle->m_vecVelocity *= fFricDeccel96;
					break;

				case 99:
					particle->m_vecVelocity *= fFricDeccel99;
					break;
			}

			if ( psystem->m_fGravitationalAcceleration > 0.0f )
			{
				if ( -50.0f * psystem->m_fGravitationalAcceleration < particle->m_vecVelocity.z )
					particle->m_vecVelocity.z -= psystem->m_fGravitationalAcceleration * timeStep;

				if ( psystem->Flags & ZCHECK_FIRST )
				{
					if ( particle->m_vecPosition.z < particle->m_fZGround )
					{
						switch ( psystem->m_Type )
						{
							case PARTICLE_RAINDROP:
							case PARTICLE_RAINDROP_SMALL:
								{
									bRemoveParticle = true;

									if ( (uint16)CGeneral::GetRandomNumber() & 1 )
									{
										AddParticle(PARTICLE_RAIN_SPLASH,
													CVector
													(
														particle->m_vecPosition.x,
														particle->m_vecPosition.y,
														0.05f + particle->m_fZGround
													),
													CVector(0.0f, 0.0f, 0.0f), NULL, 0.0f, 0, 0, 0, 0);
									}
									else
									{
										AddParticle(PARTICLE_RAIN_SPLASHUP,
													CVector
													(
														particle->m_vecPosition.x,
														particle->m_vecPosition.y,
														0.05f + particle->m_fZGround
													),
													CVector(0.0f, 0.0f, 0.0f), NULL, 0.0f, 0, 0, 0, 0);
									}

									continue;
								}
								break;

							case PARTICLE_WHEEL_WATER:
								{
									bRemoveParticle = true;

									int32 randVal = int32((uint16)CGeneral::GetRandomNumber());

									if ( randVal & 1 )
									{
										if ( (randVal % 5) == 0 )
										{
											AddParticle(PARTICLE_RAIN_SPLASH,
														CVector
														(
															particle->m_vecPosition.x,
															particle->m_vecPosition.y,
															0.05f + particle->m_fZGround
														),
														CVector(0.0f, 0.0f, 0.0f), NULL, 0.0f, 0, 0, 0, 0);
										}
										else
										{
											AddParticle(PARTICLE_RAIN_SPLASHUP,
														CVector
														(
															particle->m_vecPosition.x,
															particle->m_vecPosition.y,
															0.05f + particle->m_fZGround
														),
														CVector(0.0f, 0.0f, 0.0f), NULL, 0.0f, 0, 0, 0, 0);
										}

									}
									continue;
								}
								break;

							case PARTICLE_BLOOD:
							case PARTICLE_BLOOD_SMALL:
								{
									bRemoveParticle = true;

									CVector vecPosn = particle->m_vecPosition;
									vecPosn.z += 1.0f;

									Randomizer++;
									int32 randVal = int32(Randomizer & 7);

									if ( randVal == 5 )
									{
										int32 randTime = int32((uint16)CGeneral::GetRandomNumber());

										CShadows::AddPermanentShadow(1, gpBloodPoolTex, &vecPosn,
												0.1f, 0.0f, 0.0f, -0.1f,
												255,
												255, 0, 0,
												4.0f, (randTime & 0xFFF) + 2000, 1.0f);
									}
									else if ( randVal == 2 )
									{
										int32 randTime = int32((uint16)CGeneral::GetRandomNumber());

										CShadows::AddPermanentShadow(1, gpBloodPoolTex, &vecPosn,
												0.2f, 0.0f, 0.0f, -0.2f,
												255,
												255, 0, 0,
												4.0f, (randTime & 0xFFF) + 8000, 1.0f);
									}
									continue;
								}
								break;
						}
					}
				}
				else if ( psystem->Flags & ZCHECK_STEP )
				{
					CColPoint point;
					CEntity *entity;

					if ( CWorld::ProcessVerticalLine(particle->m_vecPosition, moveStep.z, point, entity,
														true, true, false, false, true, false, NULL) )
					{
						if ( moveStep.z <= point.point.z )
						{
							moveStep.z = point.point.z;
							if ( psystem->m_Type == PARTICLE_DEBRIS2 )
							{
								particle->m_vecVelocity.x *= 0.8f;
                                particle->m_vecVelocity.y *= 0.8f;
                                particle->m_vecVelocity.z *= -0.4f;
								if ( particle->m_vecVelocity.z < 0.005f )
									particle->m_vecVelocity.z = 0.0f;
							}
						}
					}
				}
				else if ( psystem->Flags & ZCHECK_BUMP )
				{
					if ( particle->m_vecPosition.z < particle->m_fZGround )
					{
						switch ( psystem->m_Type )
						{
							case PARTICLE_GUNSHELL_FIRST:
							case PARTICLE_GUNSHELL:
								{
									bRemoveParticle = true;

									AddParticle(PARTICLE_GUNSHELL_BUMP1,
												CVector
												(
													particle->m_vecPosition.x,
													particle->m_vecPosition.y,
													0.05f + particle->m_fZGround
												),
												CVector
												(
													CGeneral::GetRandomNumberInRange(-0.02f, 0.02f),
													CGeneral::GetRandomNumberInRange(-0.02f, 0.02f),
													CGeneral::GetRandomNumberInRange(0.05f, 0.1f)
												),
												NULL,
												particle->m_fSize, color, particle->m_nRotationStep, 0, 0, 0);

									PlayOneShotScriptObject(SCRIPT_SOUND_GUNSHELL_DROP, particle->m_vecPosition);
								}
								break;

							case PARTICLE_GUNSHELL_BUMP1:
								{
									bRemoveParticle = true;

									AddParticle(PARTICLE_GUNSHELL_BUMP2,
												CVector
												(
													particle->m_vecPosition.x,
													particle->m_vecPosition.y,
													0.05f + particle->m_fZGround
												),
												CVector(0.0f, 0.0f, CGeneral::GetRandomNumberInRange(0.03f, 0.06f)),
												NULL,
												particle->m_fSize, color, 0, 0, 0, 0);

									PlayOneShotScriptObject(SCRIPT_SOUND_GUNSHELL_DROP_SOFT, particle->m_vecPosition);
								}
								break;

							case PARTICLE_GUNSHELL_BUMP2:
								{
									bRemoveParticle = true;
									continue;
								}
								break;
						}
					}
				}
			}
			else
			{
				if ( psystem->m_fGravitationalAcceleration < 0.0f )
				{
					if ( -5.0f * psystem->m_fGravitationalAcceleration > particle->m_vecVelocity.z )
						particle->m_vecVelocity.z -= psystem->m_fGravitationalAcceleration * timeStep;
				}
				else
				{
					if ( psystem->Flags & ZCHECK_STEP )
					{
						CColPoint point;
						CEntity *entity;

						if ( CWorld::ProcessVerticalLine(particle->m_vecPosition, moveStep.z, point, entity,
														true, false, false, false, true, false, NULL) )
						{
							if ( moveStep.z <= point.point.z )
							{
								moveStep.z = point.point.z;
								if ( psystem->m_Type == PARTICLE_HELI_ATTACK )
								{
									bRemoveParticle = true;
									AddParticle(PARTICLE_STEAM, moveStep, CVector(0.0f, 0.0f, 0.05f), NULL, 0.2f, 0, 0, 0, 0);
									continue;
								}
							}
						}
					}
				}
			}

			if ( psystem->m_nFadeToBlackAmount != 0 )
			{
				if ( particle->m_nFadeToBlackTimer >= psystem->m_nFadeToBlackTime )
				{
					particle->m_nFadeToBlackTimer = 0;

					particle->m_nColorIntensity = clamp(particle->m_nColorIntensity - psystem->m_nFadeToBlackAmount,
														0, 255);
				}
				else
					++particle->m_nFadeToBlackTimer;
			}

			if ( psystem->m_nFadeAlphaAmount != 0 )
			{
				if ( particle->m_nFadeAlphaTimer >= psystem->m_nFadeAlphaTime )
				{
					particle->m_nFadeAlphaTimer = 0;

					particle->m_nAlpha = clamp(particle->m_nAlpha - psystem->m_nFadeAlphaAmount,
														0, 255);

					if ( particle->m_nAlpha == 0 )
					{
						bRemoveParticle = true;
						continue;
					}
				}
				else
					++particle->m_nFadeAlphaTimer;
			}

			if ( psystem->m_nZRotationAngleChangeAmount != 0 )
			{
				if ( particle->m_nZRotationTimer >= psystem->m_nZRotationChangeTime )
				{
					particle->m_nZRotationTimer = 0;
					particle->m_nCurrentZRotation += psystem->m_nZRotationAngleChangeAmount;
				}
				else
					++particle->m_nZRotationTimer;
			}

			if ( psystem->m_fZRadiusChangeAmount != 0.0f )
			{
				if ( particle->m_nZRadiusTimer >= psystem->m_nZRadiusChangeTime )
				{
					particle->m_nZRadiusTimer = 0;
					particle->m_fCurrentZRadius += psystem->m_fZRadiusChangeAmount;
				}
				else
					++particle->m_nZRadiusTimer;
			}

			if ( psystem->m_fAnimationSpeed != 0.0f )
			{
				if ( float(particle->m_nAnimationSpeedTimer) > psystem->m_fAnimationSpeed )
				{
					particle->m_nAnimationSpeedTimer = 0;

					if ( ++particle->m_nCurrentFrame > psystem->m_nFinalAnimationFrame )
					{
						if ( psystem->Flags & CYCLE_ANIM )
							particle->m_nCurrentFrame = psystem->m_nStartAnimationFrame;
						else
							--particle->m_nCurrentFrame;
					}
				}
				else
					++particle->m_nAnimationSpeedTimer;
			}

			if ( particle->m_nRotationStep != 0 )
			{
				particle->m_nRotation += particle->m_nRotationStep;

				if ( particle->m_nRotation >= 360 )
					particle->m_nRotation -= 360;
				else if ( particle->m_nRotation < 0 )
					particle->m_nRotation += 360;
			}

			if ( particle->m_fCurrentZRadius != 0.0f )
			{
				int32 nSinCosIndex = uint16(particle->m_nCurrentZRotation) & (SIN_COS_TABLE_SIZE - 1);

				float fX = (m_CosTable[nSinCosIndex] - m_SinTable[nSinCosIndex])
							* particle->m_fCurrentZRadius;

				float fY = (m_SinTable[nSinCosIndex] + m_CosTable[nSinCosIndex])
							* particle->m_fCurrentZRadius;

				moveStep -= particle->m_vecParticleMovementOffset;

				moveStep += CVector(fX, fY, 0.0f);

				particle->m_vecParticleMovementOffset = CVector(fX, fY, 0.0f);
			}

			particle->m_vecPosition = moveStep;
		}
	}
}

void ParticleEngine::Render()
{
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void *)rwTEXTUREADDRESSWRAP);
	RwRenderStateSet(rwRENDERSTATETEXTUREPERSPECTIVE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);

	CSprite::InitSpriteBuffer2D();

	uint32 flags = DRAW_OPAQUE;

	RwRaster *prevFrame = NULL;

	for ( int32 i = 0; i < MAX_PARTICLES; i++ )
	{
		tParticleSystemData *psystem = &mod_ParticleSystemManager.m_aParticles[i];

		bool particleBanned = false;

		CParticle *particle = psystem->m_pParticles;

		RwRaster **frames = psystem->m_ppRaster;

		tParticleType type = psystem->m_Type;

		if ( type == PARTICLE_ENGINE_SMOKE
			|| type == PARTICLE_ENGINE_SMOKE2
			|| type == PARTICLE_ENGINE_STEAM
			|| type == PARTICLE_CARFLAME_SMOKE
			|| type == PARTICLE_RUBBER_SMOKE
			|| type == PARTICLE_BURNINGRUBBER_SMOKE
			|| type == PARTICLE_EXHAUST_FUMES
			|| type == PARTICLE_CARCOLLISION_DUST )
		{
			particleBanned = true;
		}

		if ( particle )
		{
			if ( (flags & DRAW_OPAQUE) != (psystem->Flags & DRAW_OPAQUE)
				|| (flags & DRAW_DARK) != (psystem->Flags & DRAW_DARK) )
			{
				CSprite::FlushSpriteBuffer();

				if ( psystem->Flags & DRAW_OPAQUE )
				{
					RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
					RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
				}
				else
				{
					if ( psystem->Flags & DRAW_DARK )
						RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
					else
						RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDONE);

					RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDONE);
				}

				flags = psystem->Flags;
			}

			if ( frames != NULL )
			{
				RwRaster *curFrame = *frames;
				if ( curFrame != prevFrame )
				{
					CSprite::FlushSpriteBuffer();
					RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)curFrame);
					prevFrame = curFrame;
				}
			}
		}

		while ( particle != NULL )
		{
			bool canDraw = true;

			if ( particle->m_nAlpha == 0 )
				canDraw = false;


			if ( canDraw && psystem->m_nFinalAnimationFrame != 0 && frames != NULL )
			{
				RwRaster *curFrame = frames[Min(uint32(particle->m_nCurrentFrame), uint32(psystem->m_nRasterCount - 1))];
				if ( prevFrame != curFrame )
				{
					CSprite::FlushSpriteBuffer();
					RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void *)curFrame);
					prevFrame = curFrame;
				}
			}

			if ( canDraw && psystem->Flags & DRAWTOP2D )
			{
				if ( particle->m_nRotation != 0 )
				{
					 CSprite::RenderBufferedOneXLUSprite2D_Rotate_Dimension(
							particle->m_vecPosition.x,
							particle->m_vecPosition.y,
							particle->m_fSize * 63.0f,
							particle->m_fSize * 63.0f,
							particle->m_Color,
							particle->m_nColorIntensity,
							(float)particle->m_nRotation,
							particle->m_nAlpha);
				}
				else
				{
					CSprite::RenderBufferedOneXLUSprite2D(
							particle->m_vecPosition.x,
							particle->m_vecPosition.y,
							particle->m_fSize * 63.0f,
							particle->m_fSize * 63.0f,
							particle->m_Color,
							particle->m_nColorIntensity,
							particle->m_nAlpha);
				}

				canDraw = false;
			}

			if ( canDraw )
			{
				CVector coors;
				float w;
				float h;

				if ( CSprite::CalcScreenCoors(particle->m_vecPosition, &coors, &w, &h, true) )
				{
					if ( (!particleBanned || float(SCREEN_WIDTH) * fParticleScaleLimit >= w)
											&& float(SCREEN_HEIGHT) * fParticleScaleLimit >= h )
					{
						if ( particle->m_nRotation != 0 )
						{
							CSprite::RenderBufferedOneXLUSprite_Rotate_Dimension(coors.x, coors.y, coors.z,
									particle->m_fSize * w, particle->m_fSize * h,
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									float(particle->m_nRotation),
									particle->m_nAlpha);
						}
						else if ( psystem->Flags & SCREEN_TRAIL )
						{
							float fRotation;
							float fTrailLength;

							if ( particle->m_vecScreenPosition.x == 0.0f )
							{
								fTrailLength = 0.0f;
								fRotation = 0.0f;
							}
							else
							{
								CVector2D vecDist
								(
									coors.x - particle->m_vecScreenPosition.x,
									coors.y - particle->m_vecScreenPosition.y
								);

								float fDist = vecDist.Magnitude();

								fTrailLength = fDist;

								//float fRot = atan2( vecDist.x / fDist, sqrtf(1.0f - vecDist.x / fDist * (vecDist.x / fDist)) );
								float fRot = asinf(vecDist.x / fDist);

								fRotation = fRot;

								if ( vecDist.y < 0.0f )
									fRotation = -1.0f * fRot + DEGTORAD(180.0f);

								fRotation = RADTODEG(fRotation);

								if ( fRotation < 0.0f )
									fRotation += 360.0f;

								float fSpeed = particle->m_vecVelocity.Magnitude();

								float fNewTrailLength = fSpeed * (50.0f / 30.0f) * w * 2.0f;

								if ( fDist > fNewTrailLength )
									fTrailLength = fNewTrailLength;
							}

							CSprite::RenderBufferedOneXLUSprite_Rotate_Dimension(coors.x, coors.y, coors.z,
									particle->m_fSize * w,
									particle->m_fSize * h + fTrailLength * psystem->m_fTrailLengthMultiplier,
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									fRotation,
									particle->m_nAlpha);

							particle->m_vecScreenPosition = coors;
						}
						else if ( psystem->Flags & SPEED_TRAIL )
						{
							CVector vecPrevPos = particle->m_vecPosition - particle->m_vecVelocity;
							float fRotation;
							float fTrailLength;

							if ( CSprite::CalcScreenCoors(vecPrevPos, &particle->m_vecScreenPosition, &fTrailLength, &fRotation, true) )
							{
								CVector2D vecDist
								(
									coors.x - particle->m_vecScreenPosition.x,
									coors.y - particle->m_vecScreenPosition.y
								);

								float fDist = vecDist.Magnitude();

								fTrailLength = fDist;

								//float fRot = atan2(vecDist.x / fDist, sqrt(1.0f - vecDist.x / fDist * (vecDist.x / fDist)));
								float fRot = asinf(vecDist.x / fDist);

								fRotation = fRot;

								if ( vecDist.y < 0.0f )
									fRotation = -1.0f * fRot + DEGTORAD(180.0f);

								fRotation = RADTODEG(fRotation);

								if ( fRotation < 0.0f )
									fRotation += 360.0f;
							}
							else
							{
								fRotation = 0.0f;
								fTrailLength = 0.0f;
							}

							CSprite::RenderBufferedOneXLUSprite_Rotate_Dimension(coors.x, coors.y, coors.z,
									particle->m_fSize * w,
									particle->m_fSize * h + fTrailLength * psystem->m_fTrailLengthMultiplier,
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									fRotation,
									particle->m_nAlpha);
						}
						else if ( psystem->Flags & VERT_TRAIL )
						{
							float fTrailLength = fabsf(particle->m_vecVelocity.z * 10.0f);

							CSprite::RenderBufferedOneXLUSprite(coors.x, coors.y, coors.z,
									particle->m_fSize * w,
									(particle->m_fSize + fTrailLength * psystem->m_fTrailLengthMultiplier) * h, // bug ?
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									particle->m_nAlpha);
						}
						else if ( i == PARTICLE_RAINDROP_SMALL )
						{
							CSprite::RenderBufferedOneXLUSprite(coors.x, coors.y, coors.z,
									particle->m_fSize * w * 0.05f,
									particle->m_fSize * h,
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									particle->m_nAlpha);
						}
						else if ( i == PARTICLE_BOAT_WAKE )
						{
							CSprite::RenderBufferedOneXLUSprite(coors.x, coors.y, coors.z,
									particle->m_fSize * w,
									psystem->m_fDefaultInitialRadius * h,
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									particle->m_nAlpha);
						}
						else
						{
							CSprite::RenderBufferedOneXLUSprite(coors.x, coors.y, coors.z,
									particle->m_fSize * w,
									particle->m_fSize * h,
									particle->m_Color.red,
									particle->m_Color.green,
									particle->m_Color.blue,
									particle->m_nColorIntensity,
									1.0f / coors.z,
									particle->m_nAlpha);
						}
					}
				}
			}

			particle = particle->m_pNext;
		}

		CSprite::FlushSpriteBuffer();

	}

	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
}

void ParticleEngine::RemovePSystem(tParticleType type)
{
	tParticleSystemData *psystemdata = &mod_ParticleSystemManager.m_aParticles[type];

	for ( CParticle *particle = psystemdata->m_pParticles; particle; particle = psystemdata->m_pParticles )
		RemoveParticle(particle, NULL, psystemdata);
}

void ParticleEngine::RemoveParticle(CParticle *pParticle, CParticle *pPrevParticle, tParticleSystemData *pPSystemData)
{
	if ( pPrevParticle )
		pPrevParticle->m_pNext = pParticle->m_pNext;
	else
		pPSystemData->m_pParticles = pParticle->m_pNext;

	pParticle->m_pNext = m_pUnusedListHead;
	m_pUnusedListHead = pParticle;
}

void ParticleEngine::AddJetExplosion(CVector const &vecPos, float fPower, float fSize)
{
	CRGBA color(240, 240, 240, 255);

	if ( fPower < 1.0f )
		fPower = 1.0f;

	CVector vecRandOffset
	(
		CGeneral::GetRandomNumberInRange(-0.4f, 0.4f),
		CGeneral::GetRandomNumberInRange(-0.4f, 0.4f),
		CGeneral::GetRandomNumberInRange(0.1f, 0.3f)
	);

	vecRandOffset *= 2.0f;

	CVector vecStepPos = vecPos;

	for ( int32 i = 0; i < int32(fPower * 4.0f); i++ )
	{
		AddParticle(PARTICLE_EXPLOSION_MFAST,
					vecStepPos,
					CVector
					(
						CGeneral::GetRandomNumberInRange(-0.02f, 0.02f),
						CGeneral::GetRandomNumberInRange(-0.02f, 0.02f),
						CGeneral::GetRandomNumberInRange(-0.02f, 0.0f)
					),
					NULL,
					fSize, color, 0, 0, 0, 0);

		AddParticle(PARTICLE_EXPLOSION_MFAST,
					vecStepPos,
					CVector
					(
						CGeneral::GetRandomNumberInRange(-0.04f, 0.04f),
						CGeneral::GetRandomNumberInRange(-0.04f, 0.04f),
						CGeneral::GetRandomNumberInRange(0.0f, 0.07f)
					),
					NULL,
					fSize, color, 0, 0, 0, 0);

		AddParticle(PARTICLE_EXPLOSION_MFAST,
					vecStepPos,
					CVector
					(
						CGeneral::GetRandomNumberInRange(-0.04f, 0.04f),
						CGeneral::GetRandomNumberInRange(-0.04f, 0.04f),
						CGeneral::GetRandomNumberInRange(0.0f, 0.07f)
					),
					NULL,
					fSize, color, 0, 0, 0, 0);

		vecStepPos += vecRandOffset;
	}
}

void ParticleEngine::AddYardieDoorSmoke(CVector const &vecPos, CMatrix const &matMatrix)
{
	CRGBA color(0, 0, 0, 0);

	CMatrix invMat(Invert(matMatrix));

	CVector vecBasePos = matMatrix * (invMat * vecPos + CVector(0.0f, -1.0f, 0.5f));

	for ( int32 i = 0; i < 5; i++ )
	{
		CVector pos = vecBasePos;

		pos.x += CGeneral::GetRandomNumberInRange(-0.5f, 0.5f);
		pos.y += CGeneral::GetRandomNumberInRange(-0.5f, 0.5f);

		AddParticle(PARTICLE_CARCOLLISION_DUST,
					pos,
					CVector(0.0f, 0.0f, 0.0f),
					NULL,
					0.3f, color, 0, 0, 0, 0);
	}
}


}
