#pragma once
#include <string>

class CCutsceneObject;

// Native port of aap/iii_anim cutscene hands. Attachments do not consume
// CObject pool slots or change the cutscene object/savegame layout.
class CCutsceneHand
{
public:
	enum Handedness { HANDEDNESS_LEFT, HANDEDNESS_RIGHT, HANDEDNESS_BOTH, HANDEDNESS_UNDEFINED };
	enum Gender { GENDER_MALE, GENDER_FEMALE };
	enum Race { RACE_BLACK, RACE_WHITE };
	enum Stature { STATURE_0, STATURE_1 };
	enum Prop { PROP_NONE, PROP_STICK };
	static void Init();
	static void CleanUp();
	static void Attach(CCutsceneObject *object);
	static void RenderFor(CCutsceneObject *object);
	static int GetModelIndexFromName(const std::string &name);
	static bool IsCutsceneRunning(const std::string &name);
};
