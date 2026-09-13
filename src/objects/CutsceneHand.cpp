#include "common.h"
#include <vector>
#include "CutsceneHand.h"
#include "XMLConfigure.h"
#include "CutsceneObject.h"
#include "CutsceneMgr.h"
#include "FileMgr.h"
#include "ModelInfo.h"
#include "Script.h"
#include "RwHelper.h"
#include "RpAnimBlend.h"
#include "AnimBlendClumpData.h"
#include "Bones.h"
#include "TxdStore.h"

namespace {
XmlLibrary::Parser *parser;
XmlLibrary::Element *root;
int textureSlot = -1;
bool ready;
RpClump *models[2][2][2];
RpClump *stick;
RpHAnimAnimation *animations[2][2];
const char *modelNames[2][2][2] = {
	{{"SLhandCM", "SRhandCM"}, {"SLhandWM", "SRhandWM"}},
	{{"SLhandCF", "SRhandCF"}, {"SLhandWF", "SRhandWF"}}
};
const char *animationNames[2][2] = {{"SLhandWM", "SRhandWM"}, {"SLhandWF", "SRhandWF"}};
struct Hand {
	CCutsceneObject *parent;
	int side;
	RwFrame *originalFrame;
	RpClump *clump;
	RpClump *source;
	RpHAnimHierarchy *hierarchy;
	bool skinned;
	bool replaced;
};
std::vector<Hand> hands;

RwObject *SetVisibility(RwObject *object, void *data)
{
	if(RwObjectGetType(object) == rpATOMIC){
		RpAtomic *atomic = (RpAtomic*)object;
		uint32 flags = RpAtomicGetFlags(atomic);
		RpAtomicSetFlags(atomic, data ? flags | rpATOMICRENDER : flags & ~rpATOMICRENDER);
	}
	return object;
}

void OriginalVisible(Hand &hand, bool visible)
{
#ifdef PED_SKIN
	if(hand.skinned){
		if(hand.side == CCutsceneHand::HANDEDNESS_LEFT) hand.parent->SetRenderLeftHand(visible);
		else hand.parent->SetRenderRightHand(visible);
	}else
#endif
	if(hand.originalFrame)
		RwFrameForAllObjects(hand.originalFrame, SetVisibility, visible ? &hand : nil);
	hand.replaced = !visible;
}

RpClump *ReadModel(const char *name)
{
	char path[80];
	snprintf(path, sizeof(path), "anim\\%s.dff", name);
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
	if(!stream) return nil;
	RpClump *result = nil;
	if(RwStreamFindChunk(stream, rwID_CLUMP, nil, nil)) result = RpClumpStreamRead(stream);
	RwStreamClose(stream, nil);
	return result;
}

RpHAnimAnimation *ReadAnimation(const char *name)
{
	char path[80];
	snprintf(path, sizeof(path), "anim\\%s.anm", name);
	RwStream *stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, path);
	if(!stream) return nil;
	RpHAnimAnimation *result = nil;
	if(RwStreamFindChunk(stream, rwID_HANIMANIMATION, nil, nil)) result = RpHAnimAnimationStreamRead(stream);
	RwStreamClose(stream, nil);
	return result;
}

RpAtomic *BindHierarchy(RpAtomic *atomic, void *data)
{
	if(RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic)))
		RpSkinAtomicSetHAnimHierarchy(atomic, (RpHAnimHierarchy*)data);
	return atomic;
}

RwMatrix *ParentBone(Hand &hand, int bone)
{
#ifdef PED_SKIN
	if(hand.skinned){
		RpHAnimHierarchy *hier = GetAnimHierarchyFromSkinClump(hand.parent->GetClump());
		if(!hier) return nil;
		int index = RpHAnimIDGetIndex(hier, bone);
		return index < 0 ? nil : &RpHAnimHierarchyGetMatrixArray(hier)[index];
	}
#endif
	AnimBlendFrameData *frame = RpAnimBlendClumpFindFrame(hand.parent->GetClump(), ConvertBoneTag2BoneName(bone));
	return frame ? RwFrameGetLTM(frame->frame) : nil;
}

RpMaterial *SetColour(RpMaterial *material, void *data)
{
	RpMaterialSetColor(material, (RwRGBA*)data);
	return material;
}

RpAtomic *RenderAtomic(RpAtomic *atomic, void *data)
{
	if(!(RpAtomicGetFlags(atomic) & rpATOMICRENDER)) return atomic;
	// Clones share materials: restore colours immediately after each draw.
	RpGeometry *geometry = RpAtomicGetGeometry(atomic);
	std::vector<RwRGBA> colours;
	for(int i = 0; i < RpGeometryGetNumMaterials(geometry); i++)
		colours.push_back(*RpMaterialGetColor(RpGeometryGetMaterial(geometry, i)));
	uint32 flags = RpGeometryGetFlags(geometry);
	RpGeometrySetFlags(geometry, flags | rpGEOMETRYMODULATEMATERIALCOLOR);
	RpGeometryForAllMaterials(geometry, SetColour, data);
	RpAtomicRender(atomic);
	for(size_t i = 0; i < colours.size(); i++)
		RpMaterialSetColor(RpGeometryGetMaterial(geometry, i), &colours[i]);
	RpGeometrySetFlags(geometry, flags);
	return atomic;
}
}

void CCutsceneHand::CleanUp()
{
	for(size_t i = 0; i < hands.size(); i++){
		if(hands[i].replaced) OriginalVisible(hands[i], true);
		if(hands[i].clump) RpClumpDestroy(hands[i].clump);
	}
	hands.clear();
	for(int g = 0; g < 2; g++) for(int h = 0; h < 2; h++){
		if(animations[g][h]) RpHAnimAnimationDestroy(animations[g][h]);
		animations[g][h] = nil;
		for(int r = 0; r < 2; r++){
			if(models[g][r][h]) RpClumpDestroy(models[g][r][h]);
			models[g][r][h] = nil;
		}
	}
	if(stick) RpClumpDestroy(stick);
	stick = nil;
	if(textureSlot >= 0) CTxdStore::RemoveTxdSlot(textureSlot);
	textureSlot = -1;
	delete parser;
	parser = nil;
	root = nil;
	ready = false;
}

void CCutsceneHand::Init()
{
	CleanUp();
	// The supplied XML uses case-insensitive closing tags, as does iii_anim.
	std::vector<char> buffer(256 * 1024, 0);
	int file = CFileMgr::OpenFile("data\\cutscenehands.xml", "rb");
	if(!file) return;
	int length = CFileMgr::Read(file, &buffer[0], buffer.size() - 1);
	CFileMgr::CloseFile(file);
	if(length <= 0 || length >= (int)buffer.size() - 1) return;
	parser = new XmlLibrary::Parser;
	try { root = &parser->Parse(&buffer[0], length); }
	catch(const XmlLibrary::SyntaxError &) {
		debug("Cutscene hands: invalid XML; using original hands\n");
		CleanUp();
		return;
	}
	if(root->IsNull()){ CleanUp(); return; }
	CTxdStore::PushCurrentTxd();
	textureSlot = CTxdStore::AddTxdSlot("cutscenehands");
	// The filename overload retries forever when the file is missing.
	// Optional hand assets must use the non-retrying stream overload.
	RwStream *textureStream = textureSlot >= 0
		? RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, "anim\\CSHands.txd") : nil;
	bool loaded = textureStream && CTxdStore::LoadTxd(textureSlot, textureStream);
	if(textureStream) RwStreamClose(textureStream, nil);
	if(loaded){
		CTxdStore::AddRef(textureSlot);
		CTxdStore::SetCurrentTxd(textureSlot);
		for(int g = 0; g < 2; g++) for(int h = 0; h < 2; h++){
			animations[g][h] = ReadAnimation(animationNames[g][h]);
			for(int r = 0; r < 2; r++) models[g][r][h] = ReadModel(modelNames[g][r][h]);
		}
		stick = ReadModel("SRhandWFStick");
	}
	CTxdStore::PopCurrentTxd();
	if(!loaded){ CleanUp(); return; }
	ready = true;
}

void CCutsceneHand::Attach(CCutsceneObject *object)
{
	if(!ready || !object || !object->GetClump()) return;
	for(size_t i = 0; i < hands.size(); i++) if(hands[i].parent == object) return;
	for(int side = 0; side < 2; side++){
		Hand hand = {};
		hand.parent = object;
		hand.side = side;
#ifdef PED_SKIN
		hand.skinned = IsClumpSkinned(object->GetClump()) != nil;
#endif
		if(!hand.skinned){
			AnimBlendFrameData *frame = RpAnimBlendClumpFindFrame(object->GetClump(), side ? "SRhand" : "SLhand");
			if(!frame) continue;
			hand.originalFrame = frame->frame;
		}
		hands.push_back(hand);
	}
}

void CCutsceneHand::RenderFor(CCutsceneObject *object)
{
	if(!ready) return;
	bool evaluated = false;
	XMLConfigure config(*root, object->GetModelIndex());
	for(size_t i = 0; i < hands.size(); i++){
		Hand &hand = hands[i];
		if(hand.parent != object) continue;
		if(!evaluated){ config.Evaluate(); evaluated = true; }
		HandState &state = config.GetHand((Handedness)hand.side);
		RpClump *source = !strcasecmp(state.m_Model.c_str(), XMLConfigure::HAND_MODEL_STICK)
			? stick : models[state.m_Gender][state.m_Race][hand.side];
		RpHAnimAnimation *animation = animations[state.m_Gender][hand.side];
		if(state.m_Model.empty() || !source || !animation || animation->duration <= 0.0f){
			OriginalVisible(hand, true);
			continue;
		}
		if(source != hand.source){
			if(hand.clump) RpClumpDestroy(hand.clump);
			hand.clump = RpClumpClone(source);
			hand.source = nil;
			hand.hierarchy = hand.clump ? GetAnimHierarchyFromClump(hand.clump) : nil;
			if(!hand.hierarchy){ OriginalVisible(hand, true); continue; }
			RpClumpForAllAtomics(hand.clump, BindHierarchy, hand.hierarchy);
			hand.source = source;
		}
		RwMatrix *bone = ParentBone(hand, hand.side ? BONE_Rhand : BONE_Lhand);
		if(!bone){ OriginalVisible(hand, true); continue; }
#ifdef LIBRW
		if(animation->getNumNodes() != hand.hierarchy->numNodes){
			OriginalVisible(hand, true);
			continue;
		}
#endif
		RwFrame *frame = RpClumpGetFrame(hand.clump);
		RwMatrix *matrix = RwFrameGetMatrix(frame);
		*matrix = *bone;
		RwV3d offset = {state.m_vecPos.x, state.m_vecPos.y, state.m_vecPos.z};
		RwMatrix *forearm = ParentBone(hand, hand.side ? BONE_lowerarmr : BONE_lowerarml);
		if(forearm) RwV3dTransformVectors(&offset, &offset, 1, forearm);
		RwMatrixTranslate(matrix, &offset, rwCOMBINEPOSTCONCAT);
		RwV3d scale = {state.m_vecScale.x, state.m_vecScale.y, state.m_vecScale.z};
		RwMatrixScale(matrix, &scale, rwCOMBINEPRECONCAT);
		RwFrameUpdateObjects(frame);
		float time = state.m_bIsAnimStatic ? state.m_fAnimTime : CCutsceneMgr::GetCutsceneTimeInMilleseconds() * 0.001f;
		time = fmodf(Max(0.0f, time), animation->duration);
		// Reset and advance also handles backward seeks and looping with librw.
		RpHAnimHierarchySetCurrentAnim(hand.hierarchy, animation);
		RpHAnimHierarchyAddAnimTime(hand.hierarchy, time);
		RpHAnimHierarchyUpdateMatrices(hand.hierarchy);
		OriginalVisible(hand, false);
		RpClumpForAllAtomics(hand.clump, RenderAtomic, &state.m_Color);
	}
}

int CCutsceneHand::GetModelIndexFromName(const std::string &name)
{
	int id = -1;
	CModelInfo::GetModelInfo(name.c_str(), &id);
	return id;
}

bool CCutsceneHand::IsCutsceneRunning(const std::string &name)
{
	if(!strcasecmp(CCutsceneMgr::GetCutsceneName(), name.c_str())) return true;
	for(CRunningScript *script = CTheScripts::pActiveScripts; script; script = script->next){
		char scriptName[9] = {};
		memcpy(scriptName, script->m_abScriptName, 8);
		if(!strcasecmp(scriptName, name.c_str())) return true;
	}
	return false;
}
