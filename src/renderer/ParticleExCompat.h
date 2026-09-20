// Native interfaces used by the ParticleEx engines; no executable hooks.
#pragma once
#include "common.h"
#include "main.h"
#include "General.h"
#include "Timer.h"
#include "TxdStore.h"
#include "Entity.h"
#include "Sprite.h"
#include "Camera.h"
#include "Collision.h"
#include "World.h"
#include "Shadows.h"
#include "AudioScriptObject.h"
#include "ParticleObject.h"
#include "soundlist.h"

#define INJECT_PARTICLE 1
#define USE_CUSTOM_DIR 1
#define GTA3_1_1_PATCH 1
#define FIX_FLAME5_BUG 0
#define clamp(v, lo, hi) Clamp(v, lo, hi)
