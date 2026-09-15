#define FORCE_PC_SCALING
#include "common.h"
#if defined(MENU_MAP) && !defined(PS2_MENU)
#include <string>
#include <vector>
#include <sstream>
#include "MenuMap.h"
#include "MenuMapMath.h"
#include "MenuMapLabels.h"
#include "Frontend.h"
#include "Radar.h"
#include "Font.h"
#include "Text.h"
#include "Pad.h"
#include "Timer.h"
#include "main.h"
#include "Game.h"
#include "World.h"
#include "Stats.h"
#include "Zones.h"
#include "Pools.h"
#include "Vehicle.h"
#include "PlayerPed.h"
#include "FileMgr.h"
#include "Streaming.h"
#include "TxdStore.h"
#include "RwHelper.h"
#include "DMAudio.h"

namespace {
using MenuMapMath::Point;
using MenuMapMath::View;
using MenuMapMath::Bound;
struct Service { float x, y; int sprite, island; };
const Service defaultServices[] = {
	{1071.2f,-400.0f,20,0}, {345.5f,-713.5f,20,1}, {-1200.8f,-24.5f,20,2},
	{925.0f,-359.5f,18,0}, {379.0f,-493.7f,18,1}, {-1142.0f,34.7f,18,2},
	{1282.1f,-104.8f,2,0}, {380.0f,-576.6f,2,1}, {-1082.5f,55.2f,2,2},
	{893.5f,-306.1f,17,0}, {103.0f,-484.2f,17,1}, {-665.0f,-6.5f,17,2}
};
const char *englishLabels[] = {"", "Asuka Kasen", "Bomb shop", "Catalina", "Player position", "Police",
	"Donald Love", "8-Ball", "Telephone", "Telephone", "Joey Leone", "Kenji Kasen", "Telephone",
	"Luigi Goterelli", "North", "Ray Machowski", "Salvatore Leone", "Safe house", "Pay 'n' Spray",
	"Toni Cipriani", "Ammu-Nation", "Waypoint"};
struct Settings {
	CRGBA map, background, crosshair, zone;
	bool forceServices, legend, customText;
	std::vector<Service> services;
	std::u16string labels[128];
	Settings() : map(255,255,255,255), background(0,0,0,255), crosshair(234,171,54,155),
		zone(255,255,255,255), forceServices(true), legend(true), customText(false),
		services(defaultServices, defaultServices + ARRAY_SIZE(defaultServices)) {}
} settings;
bool loaded, showLegend = true, centering;
int tiles[64];
uint32 previousTime, lastClick;
Point lastClickPoint, cursor;
View view;
float screenWidth, screenHeight;
struct Legend { int sprite; CRGBA color; };
std::vector<Legend> legend;
int hoverId;
Point hoverPoint;
float hoverDistance;
CRect viewport;

float Scale(float value) { return value * SCREEN_HEIGHT / 480.0f; }
bool Chinese() {
#ifdef CHINESE
	return CGame::chineseGame;
#else
	return false;
#endif
}
std::string Trim(const std::string &s) {
	size_t first = s.find_first_not_of(" \t\r\n");
	return first == std::string::npos ? "" : s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
bool Boolean(const std::string &s) { return !strcasecmp(s.c_str(), "true") || s == "1"; }
void ReadColor(const std::string &s, CRGBA &color) {
	int r,g,b,a;
	if(sscanf(s.c_str(), "%d %d %d %d", &r,&g,&b,&a) == 4)
		color = CRGBA(Bound(r,0,255),Bound(g,0,255),Bound(b,0,255),Bound(a,0,255));
}
void ReadSettings() {
	settings = Settings();
	int file = CFileMgr::OpenFile("data\\MenuMapIII.ini", "rb");
	if(!file) return;
	char line[1024];
	std::string section;
	bool firstBlipsSection = true;
	while(CFileMgr::ReadLine(file, line, sizeof(line))) {
		std::string s = Trim(line);
		if(s.compare(0,3,"\xEF\xBB\xBF") == 0) s.erase(0,3);
		s = Trim(s.substr(0, s.find(';')));
		if(s.empty()) continue;
		if(s[0] == '[' && s.back() == ']') {
			section = s.substr(1,s.size()-2);
			if(section == "BLIPS" && firstBlipsSection) { settings.services.clear(); firstBlipsSection = false; }
			continue;
		}
		if(section == "BLIPS") {
			for(size_t i = 0; i < s.size(); i++) if(s[i] == ',' || s[i] == 'f' || s[i] == 'F') s[i] = ' ';
			std::istringstream input(s); Service entry; std::string extra;
			if(input >> entry.x >> entry.y >> entry.sprite >> entry.island && !(input >> extra)
				&& std::isfinite(entry.x) && std::isfinite(entry.y) && fabsf(entry.x) <= 2000 && fabsf(entry.y) <= 2000
				&& entry.sprite > 0 && entry.sprite < RADAR_SPRITE_COUNT && entry.island >= 0 && entry.island <= 2
				&& settings.services.size() < 128) settings.services.push_back(entry);
			continue;
		}
		size_t equal = s.find('='); if(equal == std::string::npos) continue;
		std::string key = Trim(s.substr(0,equal)), value = Trim(s.substr(equal+1));
		if(section == "GXT") {
			int id;
			if(sscanf(key.c_str(), "LG_%d", &id) == 1 && id >= 0 && id < 128)
				settings.labels[id] = MenuMapMath::DecodeUTF8(value);
		} else if(section == "MENUMAP") {
			if(key == "RadarMapColor") ReadColor(value, settings.map);
			else if(key == "BackgroundColor") ReadColor(value, settings.background);
			else if(key == "CrosshairColor") ReadColor(value, settings.crosshair);
			else if(key == "ZoneNameColor") ReadColor(value, settings.zone);
			else if(key == "ForceBlipsOnMap") settings.forceServices = Boolean(value);
			else if(key == "EnableLegendBox") settings.legend = Boolean(value);
			else if(key == "ReadStringsFromThisFile") settings.customText = Boolean(value);
		}
	}
	CFileMgr::CloseFile(file);
}
std::basic_string<wchar> Wide(const std::u16string &text) {
	std::basic_string<wchar> result;
	for(size_t i = 0; i < text.size(); i++) result += (wchar)text[i];
	// Some font scanners read one character beyond the first terminator.
	result += (wchar)0;
	return result;
}
std::basic_string<wchar> Label(int sprite) {
	int id = sprite == RADAR_SPRITE_WAYPOINT ? 108 : sprite == -1 ? 101 : sprite == -2 ? 102 : sprite;
	if(id >= 0 && id < 128 && settings.customText && !settings.labels[id].empty())
		return Wide(settings.labels[id]);
	if(Chinese()) for(size_t i = 0; i < ARRAY_SIZE(menuMapChineseLabels); i++)
		if(menuMapChineseLabels[i].id == id) return Wide(menuMapChineseLabels[i].text);
	return Wide(MenuMapMath::DecodeUTF8(sprite == -1 ? "Destination" : sprite == -2 ? "Objective" :
		sprite >= 0 && sprite < (int)ARRAY_SIZE(englishLabels) ? englishLabels[sprite] : "Marker"));
}
CRGBA Fade(CMenuManager &menu, CRGBA color) { color.a = menu.FadeIn(color.a); return color; }
float TextWidth(std::basic_string<wchar> text, float fontSize = 1.0f) {
	CFont::SetPropOn(); CFont::SetFontStyle(FONT_BANK);
	CFont::SetScale(Scale(0.34f)*fontSize,Scale(0.65f)*fontSize);
	return CFont::GetStringWidth(&text[0], true);
}
void Text(float x, float y, std::basic_string<wchar> text, float width, CRGBA color, float fontSize = 1.0f) {
	CFont::SetBackgroundOff(); CFont::SetPropOn(); CFont::SetCentreOff();
	CFont::SetRightJustifyOff(); CFont::SetJustifyOff(); CFont::SetFontStyle(FONT_BANK);
	CFont::SetWrapx(SCREEN_WIDTH - Scale(8)); CFont::SetColor(color);
	CFont::SetDropShadowPosition(0);
	float sx = Scale(0.34f)*fontSize, sy = Scale(0.65f)*fontSize;
	CFont::SetScale(sx,sy);
	float measured = CFont::GetStringWidth(&text[0], true);
	if(measured > width) CFont::SetScale(sx * width / measured, sy * width / measured);
	CFont::PrintString(x,y,&text[0]);
}
bool Inside(Point p) { return p.x >= viewport.left && p.x <= viewport.right && p.y >= viewport.top && p.y <= viewport.bottom; }
void TextureRect(RwTexture *texture, const CRect &rect, CRGBA color) {
	if(!texture || rect.right <= rect.left || rect.bottom <= rect.top) return;
	CRect clipped(Max(rect.left,viewport.left), Max(rect.top,viewport.top), Min(rect.right,viewport.right), Min(rect.bottom,viewport.bottom));
	if(clipped.right <= clipped.left || clipped.bottom <= clipped.top) return;
	float u0=(clipped.left-rect.left)/(rect.right-rect.left), v0=(clipped.top-rect.top)/(rect.bottom-rect.top);
	float u1=(clipped.right-rect.left)/(rect.right-rect.left), v1=(clipped.bottom-rect.top)/(rect.bottom-rect.top);
	CSprite2d sprite;
	sprite.m_pTexture = texture;
	sprite.Draw(clipped,color,u0,v0,u1,v0,u0,v1,u1,v1);
	sprite.m_pTexture = nil; // borrowed; TXD reference owns it
}
void DarkenLockedAreas(CMenuManager &menu) {
	// Progress masks adapted from LSDCXX's GTA III map. Redraw the clipped tile
	// textures, rather than an opaque rectangle, to preserve their alpha.
	float tileSize=view.halfSize/4, left=view.x-view.halfSize, top=view.y-view.halfSize;
	CRect oldViewport=viewport;
	std::vector<CRect> masks;
	if(!CStats::IndustrialPassed)
		masks.push_back(CRect(left+3.3f*tileSize,top+3.4f*tileSize,left+5.4f*tileSize,top+8*tileSize));
	if(!CStats::CommercialPassed) {
		masks.push_back(CRect(left,top+3.4f*tileSize,left+3.3f*tileSize,top+8*tileSize));
		masks.push_back(CRect(left,top,left+8*tileSize,top+3.4f*tileSize));
	}
	for(size_t i=0;i<masks.size();i++) {
		viewport=CRect(Max(oldViewport.left,masks[i].left),Max(oldViewport.top,masks[i].top),
			Min(oldViewport.right,masks[i].right),Min(oldViewport.bottom,masks[i].bottom));
		if(viewport.left>=viewport.right || viewport.top>=viewport.bottom)continue;
		for(int y=0;y<8;y++) for(int x=0;x<8;x++) {
			int slot=tiles[x+y*8];
			RwTexture *texture=slot>=0 && CTxdStore::GetSlot(slot)->texDict ? GetFirstTexture(CTxdStore::GetSlot(slot)->texDict) : nil;
			TextureRect(texture,CRect(left+x*tileSize,top+y*tileSize,left+(x+1)*tileSize,top+(y+1)*tileSize),Fade(menu,CRGBA(0,0,0,110)));
		}
	}
	viewport=oldViewport;
}
void LoadMap() {
	ReadSettings();
	for(int i=0;i<64;i++) {
		char name[16]; snprintf(name,sizeof(name),"radar%02d",i);
		tiles[i] = CTxdStore::FindTxdSlot(name);
		if(tiles[i] >= 0) {
			CTxdStore::AddRef(tiles[i]);
			CStreaming::RequestTxd(tiles[i],STREAMFLAGS_DEPENDENCY);
		}
	}
	CStreaming::LoadAllRequestedModels(false);
	loaded = true; centering = true; lastClick = 0;
	showLegend = settings.legend;
	previousTime = CTimer::GetTimeInMillisecondsPauseMode();
	screenWidth = SCREEN_WIDTH; screenHeight = SCREEN_HEIGHT;
	view = {SCREEN_WIDTH/2.0f,SCREEN_HEIGHT/2.0f,SCREEN_HEIGHT*2.0f};
}
bool Unlocked(int island) { return island == 0 || (island == 1 ? CStats::IndustrialPassed : CStats::CommercialPassed); }
bool NearService(const CVector &position, int sprite) {
	if(!settings.forceServices) return false;
	for(size_t i=0;i<settings.services.size();i++) {
		const Service &s=settings.services[i];
		if(s.sprite == sprite && Unlocked(s.island) && SQR(s.x-position.x)+SQR(s.y-position.y) < SQR(25.0f)) return true;
	}
	return false;
}
void Icon(int sprite, Point p, CRGBA color, float size) {
	if(sprite > 0 && sprite < RADAR_SPRITE_COUNT && CRadar::RadarSprites[sprite] && CRadar::RadarSprites[sprite]->m_pTexture) {
		TextureRect(CRadar::RadarSprites[sprite]->m_pTexture,CRect(p.x-size,p.y-size,p.x+size,p.y+size),color);
	} else if(Inside(p) && p.x-size >= viewport.left && p.x+size <= viewport.right && p.y-size >= viewport.top && p.y+size <= viewport.bottom) {
		CSprite2d::DrawRect(CRect(p.x-size,p.y-size,p.x+size,p.y+size),CRGBA(0,0,0,color.a));
		CSprite2d::DrawRect(CRect(p.x-size+Scale(2),p.y-size+Scale(2),p.x+size-Scale(2),p.y+size-Scale(2)),color);
	}
}
void Blip(CMenuManager &menu, int sprite, const CVector &pos, CRGBA color) {
	bool found=false;
	for(size_t i=0;i<legend.size();i++) if(legend[i].sprite == sprite) {found=true;break;}
	if(!found && legend.size()<64) legend.push_back({sprite,color});
	Point p=view.ToScreen(pos.x,pos.y);
	if(sprite == RADAR_SPRITE_CENTRE) {
		// North-up map: match the radar sprite orientation with its half-turn offset.
		float margin=Scale(12);
		if(p.x>=viewport.left+margin && p.x<=viewport.right-margin &&
			p.y>=viewport.top+margin && p.y<=viewport.bottom-margin)
			CRadar::DrawRotatingRadarSprite(&CRadar::CentreSprite,p.x,p.y,PI+FindPlayerHeading(),menu.FadeIn(255));
	} else
		Icon(sprite,p,Fade(menu,color),Scale(7));
	float distance=SQR(cursor.x-p.x)+SQR(cursor.y-p.y);
	if(Inside(p) && Inside(cursor) && distance<hoverDistance) {hoverId=sprite;hoverPoint=p;hoverDistance=distance;}
}
void DrawBlips(CMenuManager &menu) {
	legend.clear(); hoverId=-999; hoverDistance=SQR(Scale(13));
	for(int i=0;i<NUMRADARBLIPS;i++) {
		const sRadarTrace &trace=CRadar::ms_RadarTrace[i];
		if(!trace.m_bInUse || (trace.m_eBlipDisplay != BLIP_DISPLAY_BOTH && trace.m_eBlipDisplay != BLIP_DISPLAY_BLIP_ONLY)) continue;
		CVector pos=trace.m_vecPos; CEntity *entity=nil;
		bool attached=true;
		switch(trace.m_eBlipType) {
		case BLIP_CAR: entity=CPools::GetVehiclePool()->GetAt(trace.m_nEntityHandle); break;
		case BLIP_CHAR: entity=CPools::GetPedPool()->GetAt(trace.m_nEntityHandle); break;
		case BLIP_OBJECT: entity=CPools::GetObjectPool()->GetAt(trace.m_nEntityHandle); break;
		case BLIP_COORD: case BLIP_CONTACT_POINT: attached=false; break;
		default: continue;
		}
		if(attached) { if(!entity) continue; pos=entity->GetPosition(); }
		int sprite=trace.m_eRadarSprite;
		if(NearService(pos,sprite)) continue;
		uint32 packed=CRadar::GetRadarTraceColour(trace.m_nColor,trace.m_bDim);
		CRGBA color(packed>>24,packed>>16,packed>>8,255);
		if(sprite > 0) color=CRGBA(255,255,255,255);
		Blip(menu,sprite ? sprite : attached ? -2 : -1,pos,color);
	}
	if(settings.forceServices) for(size_t i=0;i<settings.services.size();i++) {
		const Service &s=settings.services[i];
		if(Unlocked(s.island)) Blip(menu,s.sprite,CVector(s.x,s.y,0),CRGBA(255,255,255,255));
	}
	Blip(menu,RADAR_SPRITE_CENTRE,FindPlayerCoors(),CRGBA(255,255,255,255));
}
void ProcessInput(CMenuManager &menu) {
	uint32 now=CTimer::GetTimeInMillisecondsPauseMode();
	float dt=Min((now-previousTime)*0.001f,0.05f); previousTime=now;
	if(screenWidth != SCREEN_WIDTH || screenHeight != SCREEN_HEIGHT) {
		view.x *= SCREEN_WIDTH/screenWidth; view.y *= SCREEN_HEIGHT/screenHeight; view.halfSize *= SCREEN_HEIGHT/screenHeight;
		screenWidth=SCREEN_WIDTH; screenHeight=SCREEN_HEIGHT;
	}
	CPad *pad=CPad::GetPad(0);
	// The map bypasses normal menu navigation, including its mouse-hiding
	// logic. Select the cursor before panning, zooming or placing a waypoint.
	bool mouseInput = pad->GetMouseX() != 0.0f || pad->GetMouseY() != 0.0f ||
		pad->GetLeftMouse() || pad->GetRightMouse() || pad->GetMouseWheelUp() || pad->GetMouseWheelDown();
	bool controllerInput = abs(pad->GetLeftStickX()) > 12 || abs(pad->GetLeftStickY()) > 12 ||
		pad->GetDPadLeft() || pad->GetDPadRight() || pad->GetDPadUp() || pad->GetDPadDown() ||
		pad->GetLeftShoulder2() || pad->GetRightShoulder2() || pad->GetSquareJustDown() || pad->GetLeftShoulder1JustDown();
	if(mouseInput)
		menu.m_bShowMouse = true;
	else if(controllerInput) {
		menu.m_bShowMouse = false;
		lastClick = 0;
	}
	cursor = menu.m_bShowMouse ? Point{(float)menu.m_nMousePosX,(float)menu.m_nMousePosY} : Point{SCREEN_WIDTH/2.0f,(viewport.top+viewport.bottom)/2};
	if(menu.m_nMenuFadeAlpha >= 255) {
		if(pad->GetCharJustDown('L') || pad->GetLeftShoulder1JustDown()) showLegend=!showLegend;
		if(pad->GetCharJustDown('R')) centering=true;
		if(menu.m_bShowMouse && Inside(cursor) && pad->GetLeftMouseJustDown()) {
			if(lastClick && now-lastClick<=350 && SQR(cursor.x-lastClickPoint.x)+SQR(cursor.y-lastClickPoint.y)<SQR(Scale(8))) {centering=true;lastClick=0;}
			else {lastClick=now;lastClickPoint=cursor;}
		}
		if(menu.m_bShowMouse && Inside(cursor) && pad->GetLeftMouse() && !pad->GetLeftMouseJustDown()) {
			float dx=menu.m_nMousePosX-menu.m_nMouseOldPosX, dy=menu.m_nMousePosY-menu.m_nMouseOldPosY;
			view.x+=dx;view.y+=dy;if(dx || dy)centering=false;
		}
		float right=(pad->GetRight()||pad->GetDPadRight()?1.0f:0)-(pad->GetLeft()||pad->GetDPadLeft()?1.0f:0);
		float down=(pad->GetDown()||pad->GetDPadDown()?1.0f:0)-(pad->GetUp()||pad->GetDPadUp()?1.0f:0);
		if(abs(pad->GetLeftStickX())>12) right+=pad->GetLeftStickX()/128.0f;
		if(abs(pad->GetLeftStickY())>12) down+=pad->GetLeftStickY()/128.0f;
		view.x-=Bound(right,-1,1)*Scale(500)*dt; view.y-=Bound(down,-1,1)*Scale(500)*dt;
		if(right || down) centering=false;
		float factor=1.0f;
		if(pad->GetMouseWheelUp())factor=1.15f;
		else if(pad->GetMouseWheelDown())factor=1.0f/1.15f;
		else if(pad->GetPageUp()||pad->GetRightShoulder2())factor=expf(dt*1.8f);
		else if(pad->GetPageDown()||pad->GetLeftShoulder2())factor=expf(-dt*1.8f);
		if(factor!=1.0f) {view.Zoom(factor,Inside(cursor)?cursor:Point{SCREEN_WIDTH/2.0f,SCREEN_HEIGHT/2.0f},SCREEN_HEIGHT*0.5f,SCREEN_HEIGHT*8.0f);centering=false;}
	}
	if(centering) {
		CVector player=FindPlayerCoors();
		Point p=view.ToScreen(player.x,player.y);
		float dx=SCREEN_WIDTH/2.0f-p.x,dy=(viewport.top+viewport.bottom)/2-p.y;
		float amount=1.0f-expf(-12.0f*dt);view.x+=dx*amount;view.y+=dy*amount;
		if(fabsf(dx)+fabsf(dy)<0.5f)centering=false;
	}
	float midY=(viewport.top+viewport.bottom)/2;
	view.x=Bound(view.x,SCREEN_WIDTH/2.0f-view.halfSize,SCREEN_WIDTH/2.0f+view.halfSize);
	view.y=Bound(view.y,midY-view.halfSize,midY+view.halfSize);
	if(menu.m_nMenuFadeAlpha >= 255 && Inside(cursor) && (pad->GetRightMouseJustDown()||pad->GetSquareJustDown()||pad->GetCharJustDown('T'))) {
		Point pos=view.ToWorld(cursor.x,cursor.y);
		if(fabsf(pos.x)<=2000 && fabsf(pos.y)<=2000) {
			CRadar::ToggleTargetMarker(pos.x,pos.y);
			DMAudio.PlayFrontEndSound(SOUND_FRONTEND_MENU_SETTING_CHANGE,0);
		}
	}
}
}

void CMenuMap::Shutdown() {
	if(!loaded) return;
	// Drop only our references. Normal radar streaming evicts unused tiles.
	for(int i=0;i<64;i++) if(tiles[i]>=0) CTxdStore::RemoveRefWithoutDelete(tiles[i]);
	loaded=false; legend.clear();
}

void CMenuMap::Draw(CMenuManager &menu) {
	if(!FindPlayerPed()) return;
	if(!loaded) LoadMap();
	CFontDetails savedFont=CFont::Details;
	viewport=CRect(0,0,SCREEN_WIDTH,SCREEN_HEIGHT);
	ProcessInput(menu);
	CMenuManager::fMapCenterX=view.x; CMenuManager::fMapCenterY=view.y; CMenuManager::fMapSize=view.halfSize;
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,(void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE,(void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE,(void*)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND,(void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND,(void*)rwBLENDINVSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEFOGENABLE,(void*)FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER,(void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS,(void*)rwTEXTUREADDRESSCLAMP);
	CSprite2d::DrawRect(viewport,Fade(menu,settings.background));
	float size=view.halfSize/4;
	for(int y=0;y<8;y++) for(int x=0;x<8;x++) {
		int slot=tiles[x+y*8];
		RwTexture *texture=slot>=0 && CTxdStore::GetSlot(slot)->texDict ? GetFirstTexture(CTxdStore::GetSlot(slot)->texDict) : nil;
		CRect tile(view.x-view.halfSize+x*size,view.y-view.halfSize+y*size,view.x-view.halfSize+(x+1)*size,view.y-view.halfSize+(y+1)*size);
		TextureRect(texture,tile,Fade(menu,settings.map));
	}
	DarkenLockedAreas(menu);
	DrawBlips(menu);
	if(Inside(cursor)) {
		CRGBA col=Fade(menu,settings.crosshair);float t=Scale(0.65f);
		CSprite2d::DrawRect(CRect(cursor.x-t,viewport.top,cursor.x+t,viewport.bottom),col);
		CSprite2d::DrawRect(CRect(viewport.left,cursor.y-t,viewport.right,cursor.y+t),col);
	}
	if(showLegend && settings.legend) {
		int columns=Min((int)legend.size(),2),rows=((int)legend.size()+1)/2;
		float width=Scale(200*columns+8);
		float left=(SCREEN_WIDTH-width)/2, top=Scale(70);
		CSprite2d::DrawRect(CRect(left,top,left+width,top+Scale(rows*22+12)),Fade(menu,CRGBA(0,0,0,180)));
		for(size_t i=0;i<legend.size();i++) {
			float x=left+Scale(15+(i%columns)*200),y=top+Scale(16+(i/columns)*22);
			Icon(legend[i].sprite,{x,y},Fade(menu,legend[i].color),Scale(7.5f));
			Text(x+Scale(15),y-Scale(8),Label(legend[i].sprite),Scale(168),Fade(menu,CRGBA(255,255,255,255)),1.25f);
		}
	}
	if(hoverId!=-999 && menu.m_bShowMouse) {
		std::basic_string<wchar> label=Label(hoverId);
		float textWidth=Min(TextWidth(label),Scale(200));
		float width=textWidth+Scale(12);
		float x=Bound(hoverPoint.x+Scale(12),Scale(8),SCREEN_WIDTH-width-Scale(8));
		float y=Bound(hoverPoint.y-Scale(29),viewport.top,viewport.bottom-Scale(25));
		CSprite2d::DrawRect(CRect(x,y,x+width,y+Scale(20)),Fade(menu,CRGBA(0,0,0,105)));
		Text(x+Scale(6),y+Scale(3),label,textWidth,Fade(menu,CRGBA(255,230,160,255)));
	}
	// Match the original re3 map footer; the native page title is drawn next.
	CSprite2d::DrawRect(CRect(SCREEN_SCALE_X(14.0f), SCREEN_SCALE_FROM_BOTTOM(95.0f),
		SCREEN_WIDTH-SCREEN_SCALE_X(11.0f), SCREEN_SCALE_FROM_BOTTOM(59.0f)), Fade(menu,CRGBA(235,170,50,255)));
	const std::u16string help[] = {
		Chinese() ? u"\u6eda\u8f6e / PgUp PgDn: \u7f29\u653e" : u"Wheel / PgUp PgDn: zoom",
		Chinese() ? u"\u62d6\u52a8 / \u65b9\u5411\u952e: \u79fb\u52a8   \u53f3\u952e / T: \u5bfc\u822a\u70b9" : u"Drag / arrows: move   RMB / T: waypoint",
		Chinese() ? u"\u53cc\u51fb / R: \u73a9\u5bb6   L: \u56fe\u4f8b   Esc: \u8fd4\u56de" : u"Double-click / R: player   L: legend   Esc: return"
	};
	for(int i=0;i<3;i++)
		Text(SCREEN_SCALE_X(30.0f),SCREEN_SCALE_FROM_BOTTOM(95.0f-i*11.0f),Wide(help[i]),
			SCREEN_WIDTH-SCREEN_SCALE_X(150.0f),Fade(menu,CRGBA(0,0,0,255)));
	// Track the cursor's world position without covering the footer or logo.
	Point world=view.ToWorld(cursor.x,cursor.y);
	CVector position(world.x,world.y,0);
	if(Inside(cursor) && fabsf(world.x)<=2000 && fabsf(world.y)<=2000) {
		CZone *zone=CTheZones::FindSmallestZonePositionType(&position,ZONE_NAVIG);
		if(!zone) zone=CTheZones::FindSmallestZonePositionType(&position,ZONE_DEFAULT);
		if(zone) {
			std::basic_string<wchar> name(zone->GetTranslatedName());
			const float zoneFontSize=1.35f;
			float width=Min(TextWidth(name,zoneFontSize),Scale(270));
			float x=SCREEN_WIDTH-SCREEN_SCALE_X(20)-width;
			float y=SCREEN_SCALE_FROM_BOTTOM(120.0f);
			Text(x+Scale(1),y+Scale(1),name,width,Fade(menu,CRGBA(0,0,0,110)),zoneFontSize);
			Text(x,y,name,width,Fade(menu,settings.zone),zoneFontSize);
		}
	}
	CFont::DrawFonts();CFont::Details=savedFont;
}
#endif
