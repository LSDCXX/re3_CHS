"""Compile production font ink measurement with asymmetric synthetic atlases."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/renderer/Font.cpp').read_text(encoding='utf-8-sig')
start = source.index('CFont::PrintHelpString(float x, float y, wchar *text)')
brace = source.index('{', start)
level, end = 1, brace + 1
while level:
    level += (source[end] == '{') - (source[end] == '}')
    end += 1
function = 'void\n' + source[start:end]
prelude = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iostream>
using uint8=uint8_t; using uint32=uint32_t; using wchar=uint16_t; using bool16=uint16_t;
#define nil nullptr
#define FIX_BUGS
#define LIBRW
#define MORE_LANGUAGES
#define CHINESE
#define BUTTON_ICONS
template<class T> T Min(T a,T b){return std::min(a,b);}
template<class T> T Max(T a,T b){return std::max(a,b);}
struct CRect{float left,top,right,bottom; CRect()=default;
 CRect(float l,float t,float r,float b):left(l),top(t),right(r),bottom(b){}};
struct CRGBA{int r=0,g=0,b=0,a=255;};
static int readbacks=0,destroyed=0;
struct RwImage{int width=16,height=16,depth=32;std::vector<uint8> pixels;void convertTo32(){}};
struct RwRaster{RwImage image;RwImage *toImage(){++readbacks;return new RwImage(image);}};
int RwImageGetDepth(RwImage *p){return p->depth;}
int RwImageGetWidth(RwImage *p){return p->width;}
int RwImageGetHeight(RwImage *p){return p->height;}
int RwImageGetStride(RwImage *p){return p->width*4;}
uint8 *RwImageGetPixels(RwImage *p){return p->pixels.data();}
void RwImageDestroy(RwImage *p){++destroyed;delete p;}
static float screenX=1,screenY=1;
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720
#define SCREEN_SCALE_X(x) ((x)*screenX)
#define SCREEN_SCALE_Y(y) ((y)*screenY)
'''
body = r'''
struct CFontDetails{
 float scaleX=1,scaleY=1,wrapX=70,slant=0,slantRefX=0,slantRefY=0;
 int style=0,bank=0,dropShadowPosition=0;
 bool proportional=true,background=true,bFontHalfTexture=false;
 CRGBA backgroundColor,color;
};
struct CFontRenderState{int dummy=0;};
struct CGame{static bool chineseGame;};bool CGame::chineseGame=false;
static int gChineseFontStyleOverride=-1;static float gChineseSyntheticSlant=0;
static CRect background;static int backgrounds=0,drawn=0;
struct CSprite2d{static void DrawRect(const CRect &r,const CRGBA &){background=r;++backgrounds;}};
int UnicodeStrlen(const wchar *s){int n=0;while(s[n])++n;return n;}
static RwRaster atlas;
struct CFont{
 static CFontDetails Details;static CFontRenderState RenderState;
 static uint8 LanguageSet;static bool16 NewLine;static int PS2Symbol;
 static void DrawFonts(){}static void PrintHelpString(float,float,wchar*);
 static void PrintString(float x,float y,wchar *text){
  const float start=x;assert(!Details.background);
  while(*text){
   bool button=false;
   if(*text=='~'){
    const wchar token=text[1];text+=3;
    if(token=='n'){x=start;y+=18*Details.scaleY;continue;}
    if(token!='X')continue;button=true;
   }else if(*text==' '){x+=6*Details.scaleX;++text;continue;}else ++text;
   const float advance=(button?17:9)*Details.scaleX;
   if(x>start && x+advance>Details.wrapX){x=start;y+=18*Details.scaleY;}
   CRect rect(x,y,x+16*Details.scaleX,y+16*Details.scaleY);
   if(gHelpTextBounds.active)gHelpTextBounds.Capture(&atlas,rect,0,0,1,1);else ++drawn;
   x+=advance;
  }
 }
};
CFontDetails CFont::Details;CFontRenderState CFont::RenderState;
uint8 CFont::LanguageSet=0;bool16 CFont::NewLine=0;int CFont::PS2Symbol=0;
static bool near(float a,float b){return std::fabs(a-b)<0.001f;}
'''
tests = r'''
int main(){
 atlas.image.pixels.assign(16*16*4,0);
 for(int y=2;y<14;++y)for(int x=3;x<11;++x)atlas.image.pixels[(y*16+x)*4+3]=255;
 gHelpTextBounds.Begin();
 gHelpTextBounds.Capture(&atlas,CRect(10,20,42,52),0,0,1,1);
 assert(near(gHelpTextBounds.bounds.left,16));assert(near(gHelpTextBounds.bounds.right,32));
 gHelpTextBounds.Capture(&atlas,CRect(50,20,82,52),0,0,1,1);
 assert(readbacks==1);assert(near(gHelpTextBounds.bounds.right,72));
 gHelpTextBounds.End();assert(destroyed==1);
 RwRaster empty;empty.image.pixels.assign(16*16*4,0);
 gHelpTextBounds.Begin();gHelpTextBounds.Capture(&empty,CRect(0,0,16,16),0,0,1,1);
 assert(!gHelpTextBounds.valid);gHelpTextBounds.End();
 gHelpTextBounds.Begin();gHelpTextBounds.Capture(&atlas,CRect(0,0,20,20),0.2f,0.1f,0.7f,0.9f);
 assert(gHelpTextBounds.bounds.left>=0 && gHelpTextBounds.bounds.right<=20);gHelpTextBounds.End();
 gHelpTextBounds.Begin();gHelpTextBounds.Capture(nullptr,CRect(1,2,11,12),0,0,1,1);
 assert(near(gHelpTextBounds.bounds.left,1));gHelpTextBounds.End();
 const wchar mixed[]={'A','~','r','~',0x4e2d,' ','~','X','~','~','n','~','B','B','B','B','B','B','B','B',
                      '~','n','~','(',0x4e2d,')',' ',' ',0};
 std::vector<wchar> text(mixed,mixed+sizeof(mixed)/sizeof(*mixed));
 CFont::PrintHelpString(10,20,text.data());
 assert(backgrounds==1 && drawn>0 && CFont::Details.background && !gHelpTextBounds.active);
 const CRect first=background;const int reads=readbacks;
 CFont::Details.color.a=42;CFont::Details.backgroundColor.a=32;
 CFont::PrintHelpString(10,30,text.data());
 assert(readbacks==reads && near(background.left,first.left) && near(background.top,first.top+10));
 assert(near(background.left,9));assert(near(background.right,70));
 assert(near(13-background.left,4));assert(near(background.right-66,4));
 text[0]='C';CFont::PrintHelpString(10,30,text.data());assert(readbacks>reads);
 int before=readbacks;gHelpTextBounds.Reset();CFont::PrintHelpString(10,30,text.data());assert(readbacks>before);
 before=readbacks;CFont::Details.scaleX=1.5f;screenX=1.5f;
 CFont::PrintHelpString(10,30,text.data());assert(readbacks>before);
 assert(near(14.5f-background.left,6));assert(readbacks==destroyed);
 int boxes=backgrounds;wchar blank[]={0};CFont::PrintHelpString(10,30,blank);assert(backgrounds==boxes);
 wchar spaces[]={' ',' ',0};CFont::PrintHelpString(10,30,spaces);assert(backgrounds==boxes);
 std::cout<<"help bounds: asymmetric ink, multiline, mixed text, tokens, crop, cache and scale passed\n";
}
'''
header = '#include "' + (root / 'src/renderer/FontHelpBounds.h').as_posix() + '"\n'
with tempfile.TemporaryDirectory(prefix='help-bounds-') as directory:
    directory = Path(directory)
    harness = directory / 'help.cpp'
    harness.write_text(prelude + header + body + function + '\n' + tests, encoding='utf-8')
    executable = directory / 'help.exe'
    subprocess.run(['cl', '/nologo', '/EHsc', '/std:c++14', str(harness), '/Fe:' + str(executable)], cwd=directory, check=True)
    subprocess.run([str(executable)], check=True)
