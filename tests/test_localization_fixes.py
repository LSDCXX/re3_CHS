"""Compile production helpers to check units, mouse, scripts and directory APIs."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]

def function(file, marker):
    source = (root / file).read_text(encoding='utf-8')
    first = source.index(marker)
    brace = source.index('{', first)
    level = 1
    last = brace + 1
    while level:
        level += (source[last] == '{') - (source[last] == '}')
        last += 1
    return source[first:last]

units = 'bool\n' + function('src/core/re3.cpp', 'Localization::UsesMetricUnits(void)')
mouse = 'float\n' + function('src/core/Camera.h', 'GetMouseYAcceleration(void) const')
rectangle = 'static CRect\n' + function('src/renderer/Hud.cpp', 'ScaleScriptRectangle(const CRect &rect)')
sprite = 'static void\n' + function('src/renderer/Hud.cpp', 'DrawScriptSprite(CSprite2d &sprite, const CRect &rect, const CRGBA &color)')
helpers = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <string>
#include <iostream>
typedef int32_t int32;
typedef uint32_t uint32;
typedef unsigned char uint8;
typedef uintptr_t uintptr;
#define nil nullptr
#define LOCALE_USER_DEFAULT 0x400
#define LOCALE_IMEASURE 0xD
#define LOCALE_RETURN_NUMBER 0x20000000
static uint32 systemUnits = 0;
static bool systemAvailable = true;
static int localeCalls = 0;
int GetLocaleInfoA(uint32 locale, uint32 flags, char *data, int size)
{
    assert(locale == LOCALE_USER_DEFAULT);
    assert(flags == (LOCALE_IMEASURE | LOCALE_RETURN_NUMBER));
    assert(size == sizeof(uint32));
    localeCalls++;
    if(!systemAvailable) return 0;
    std::memcpy(data, &systemUnits, sizeof(systemUnits));
    return 1;
}
#include "Localization.h"
int32 Localization::Units = Localization::UNITS_SYSTEM;
'''+units+r'''
struct Camera {
    float m_fMouseAccelHorzntl = 0.0025f;
    float m_fMouseAccelVertical = 0.003f;
'''+mouse+r'''
};
struct CRect {
    float left, bottom, right, top;
    CRect(float l,float b,float r,float t):left(l),bottom(b),right(r),top(t){}
};
struct CRGBA {};
#define SCALE_AND_CENTER_X(x) ((x)*2.0f+40.0f)
#define SCREEN_SCALE_Y(y) ((y)*3.0f)
enum {rwFILTERNEAREST=1,rwFILTERLINEAR=2,rwRENDERSTATETEXTUREFILTER=3};
static uint32 filter = rwFILTERNEAREST;
bool RwRenderStateGet(int state,void *out){assert(state==rwRENDERSTATETEXTUREFILTER); *(uint32*)out=filter; return true;}
bool RwRenderStateSet(int state,void *value){assert(state==rwRENDERSTATETEXTUREFILTER); filter=(uint32)(uintptr)value; return true;}
struct CSprite2d {
    uint32 seenFilter = 0;
    void Draw(const CRect&,const CRGBA&){seenFilter=filter;}
};
'''+rectangle+'\n'+sprite+r'''
int main()
{
    using namespace Localization;
    assert(ResolveMetricUnits(UNITS_SYSTEM,0,true));
    assert(!ResolveMetricUnits(UNITS_SYSTEM,1,true));
    assert(ResolveMetricUnits(UNITS_SYSTEM,1,false));
    assert(ResolveMetricUnits(UNITS_METRIC,1,true));
    assert(!ResolveMetricUnits(UNITS_IMPERIAL,0,true));
#ifdef FIX_BUGS
    systemUnits=0; assert(UsesMetricUnits());
    systemUnits=1; assert(!UsesMetricUnits());
    systemAvailable=false; assert(UsesMetricUnits());
    localeCalls=0;
    Units=UNITS_METRIC; assert(UsesMetricUnits());
    Units=UNITS_IMPERIAL; assert(!UsesMetricUnits());
    assert(localeCalls==0);
    Units=UNITS_SYSTEM; systemAvailable=true; systemUnits=1;
    assert(!UsesMetricUnits()); assert(localeCalls==1);
#else
    assert(UsesMetricUnits());
#endif
    Camera camera;
#ifdef FIX_BUGS
    assert(camera.GetMouseYAcceleration()==camera.m_fMouseAccelHorzntl);
    camera.m_fMouseAccelVertical=99.0f;
    assert(camera.GetMouseYAcceleration()==camera.m_fMouseAccelHorzntl);
#else
    assert(camera.GetMouseYAcceleration()==camera.m_fMouseAccelVertical);
#endif
    const CRect input(10,20,30,40);
    const CRect output=ScaleScriptRectangle(input);
#ifdef FIX_BUGS
    assert(output.left==60 && output.bottom==60 && output.right==100 && output.top==120);
#else
    assert(output.left==10 && output.bottom==20 && output.right==30 && output.top==40);
#endif
    CSprite2d image;
    DrawScriptSprite(image,output,CRGBA());
#ifdef FIX_BUGS
    assert(image.seenFilter==rwFILTERLINEAR);
#else
    assert(image.seenFilter==rwFILTERNEAREST);
#endif
    assert(filter==rwFILTERNEAREST);
    std::cout << "Units, mouse and script rendering checks passed\n";
}
'''

helpers = helpers.replace('#include "Localization.h"', (root/'src/core/Localization.h').read_text(encoding='utf-8').replace('#include "common.h"', ''))

directory_prefix = r'''
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
typedef char CHAR;
#define MAX_PATH 260
#define nil nullptr
#define CSIDL_MYDOCUMENTS 5
#define SHGFP_TYPE_CURRENT 0
#define SUCCEEDED(x) ((x)>=0)
static bool folderSuccess=true;
static std::string documents="D:/Redirected Documents";
static std::string created;
int SHGetFolderPathA(void*,int folder,void*,int flags,char *buffer)
{
    assert(folder==CSIDL_MYDOCUMENTS && flags==SHGFP_TYPE_CURRENT);
    if(!folderSuccess) return -1;
    std::strcpy(buffer,documents.c_str());
    return 0;
}
void _psCreateFolder(const char *path){created=path;}
'''

for source in ('src/skel/win/win.cpp','src/skel/glfw/glfw.cpp','src/core/main.cpp','src/core/Frontend.cpp','src/core/Frontend_PS2.cpp'):
    if not (root/source).exists():
        continue
    stack=[]
    for line in (root/source).read_text(encoding='utf-8').splitlines():
        text=line.strip()
        if text.startswith(('#if ', '#ifdef ', '#ifndef ')):
            stack.append(text)
        elif text.startswith('#else') and stack:
            stack[-1]='else '+stack[-1]
        elif text.startswith('#endif') and stack:
            stack.pop()
        if re.search(r'CGame::(?:frenchGame|germanGame|noProstitutes) = true;',line) or re.search(r'CGame::nastyGame = false;',line):
            assert '#ifndef FIX_BUGS' in stack,(source,line)

assert 'm_fMultiHori' not in (root/'src/core/Cam.cpp').read_text(encoding='utf-8')
assert 'TheCamera.m_fMouseAccelVertical' not in (root/'src/core/Cam.cpp').read_text(encoding='utf-8')

with tempfile.TemporaryDirectory(prefix='localization-fixes-') as temp:
    directory=Path(temp)
    tasks=[('helpers-on',helpers,['/DFIX_BUGS','/DUSE_MEASUREMENTS_IN_METERS']),
           ('helpers-off',helpers,['/DUSE_MEASUREMENTS_IN_METERS'])]
    game='GTA3' if root.name.startswith('re3') else 'GTA Vice City'
    for backend in ('win','glfw'):
        source=(root/f'src/skel/{backend}/{backend}.cpp').read_text(encoding='utf-8')
        assert 'RegOpenKeyEx' not in source and 'RegQueryValueEx' not in source
        body='const char *'+function(f'src/skel/{backend}/{backend}.cpp','_psGetUserFilesFolder()')
        test=directory_prefix+body+r'''
int main(){
#ifdef USE_MY_DOCUMENTS
    std::string expected=documents+"\\GAME User Files";
    assert(_psGetUserFilesFolder()==expected); assert(created==expected);
    folderSuccess=false; assert(std::string(_psGetUserFilesFolder())=="data");
    folderSuccess=true; documents=std::string(250,'x');
    assert(std::string(_psGetUserFilesFolder())=="data");
#else
    assert(std::string(_psGetUserFilesFolder())=="userfiles");
    assert(created=="userfiles");
#endif
    std::cout << "Directory API checks passed\n";
}
'''
        test=test.replace('GAME',game)
        tasks.extend([(backend+'-docs',test,['/DUSE_MY_DOCUMENTS']),(backend+'-local',test,[])])
    for name,cpp,definitions in tasks:
        path=directory/(name+'.cpp');path.write_text(cpp,encoding='utf-8')
        binary=directory/(name+'.exe')
        command=[os.environ.get('CXX','cl'),'/nologo','/EHsc','/std:c++17','/I'+str(root/'src/core'),*definitions,str(path),'/Fe:'+str(binary)]
        subprocess.run(command,cwd=directory,check=True)
        subprocess.run([str(binary)],check=True)
print('Regional restrictions and all six native-helper configurations passed')
