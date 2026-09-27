"""Compile regression tests against extracted production CPad methods.

Run from a VS x64 developer prompt: python tests/test_pad_input.py
Or set CXX to a C++ compiler (g++, clang++ or cl).
Cheat side effects are stubbed; matching, edge detection, reconciliation and
pedal methods are the actual production method bodies.
"""
from pathlib import Path
import json
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root/'src/core/Pad.cpp').read_text(encoding='utf-8')
header = (root/'src/core/Pad.h').read_text(encoding='utf-8')
vc = (root/'src/core/PlayerHealth.h').exists()

def method(signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

state = header[header.index('class CControllerState'):header.index('VALIDATE_SIZE(CControllerState')]
cheats = method('void CPad::AddToCheatString(char c)')
# Replace effects, leaving the original comparisons, ordering and guards intact.
cheats = re.sub(r'(\n\s*(?:if|else if)\s*\([^\n]+\)\s*\n)\s*([^\n;]+);',
                lambda m: m[1]+'\t\tHit('+json.dumps(m[2].strip())+');', cheats)
cpp = '''
#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <iostream>
#include "PadInput.h"
using int16 = short;
using int32 = int;
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define CURMODE Mode
#define MASTER
#define FIX_BUGS
template<class T> T Max(T a,T b) { return std::max(a,b); }
template<class T> T Min(T a,T b) { return std::min(a,b); }
std::string hit;
int hits = 0;
void Hit(const char* action) { hit = action; ++hits; }
bool replay = false;
struct CRecordDataForGame { static bool IsPlayingBack() { return replay; } };
struct CRecordDataForChase { static bool ShouldThisPadBeLeftAlone(int) { return false; } };
'''+state+'''
CControllerState CheatPadState{}, OldCheatPadState{};
struct CPad {
    CControllerState NewState{}, OldState{};
    char CheatString[12];
    int Mode = 0;
    bool standard = true, disabled = false;
    bool IsStandardControls() { return standard; }
    bool ArePlayerControlsDisabled() { return disabled; }
    CControllerState ReconcileTwoControllersInput(const CControllerState&, const CControllerState&);
    void AddToCheatString(char);
    void DoCheats(int16);
    int16 GetAccelerate();
    int16 GetBrake();
};
'''
cpp += '\n'.join([method('void\nCControllerState::Clear(void)'),
                     method('CControllerState CPad::ReconcileTwoControllersInput'),
                     method('int16 CPad::GetAccelerate(void)'),
                     method('int16 CPad::GetBrake(void)'), cheats,
                     method('void CPad::DoCheats(int16 unk)')])
cpp += '''
void Frame(CPad& pad, CControllerState state) {
    OldCheatPadState = CheatPadState;
    CheatPadState = state;
    pad.DoCheats(0);
}
void Code(CPad& pad, const char* sequence, const char* expected) {
    memset(pad.CheatString, ' ', sizeof(pad.CheatString));
    hit.clear(); hits = 0;
    Frame(pad, {});
    for (const char* c = sequence; *c; ++c) {
        CControllerState state{};
        switch (*c) {
        case '1': state.LeftShoulder1 = 255; break;
        case '2': state.LeftShoulder2 = 80; break;
        case '3': state.RightShoulder1 = 255; break;
        case '4': state.RightShoulder2 = 80; break;
        case 'T': state.Triangle = 255; break;
        case 'C': state.Circle = 255; break;
        case 'X': state.Cross = 255; break;
        case 'S': state.Square = 255; break;
        case 'U': state.DPadUp = 255; break;
        case 'D': state.DPadDown = 255; break;
        case 'L': state.DPadLeft = 255; break;
        case 'R': state.DPadRight = 255; break;
        }
        Frame(pad, state);
        // Holding a button and multiple evaluations cannot duplicate an edge.
        Frame(pad, state);
        pad.DoCheats(0);
        Frame(pad, {});
    }
    assert(hit == expected && hits == 1);
}
int main() {
    CPad pad;
    CControllerState idle{};
    int last = -1;
    for (int pressure = 0; pressure <= 255; ++pressure) {
        CControllerState joy{};
        joy.RightShoulder2 = joy.LeftShoulder2 = pressure;
        pad.NewState = pad.ReconcileTwoControllersInput(idle, joy);
        pad.NewState = pad.ReconcileTwoControllersInput(idle, pad.NewState);
        assert(pad.NewState.RightShoulder2 == pressure);
        assert(pad.NewState.LeftShoulder2 == pressure);
        int gas = pad.GetAccelerate();
        assert(gas >= last && gas <= 255);
        assert(pad.GetBrake() == gas);
        if (pressure <= 30) assert(gas == 0);
        if (pressure == 128) assert(gas == 111);
        if (pressure == 255) assert(gas == 255);
        last = gas;
    }
    CControllerState key{}, joy{};
    key.RightShoulder2 = 255; joy.RightShoulder2 = 80;
    pad.NewState = pad.ReconcileTwoControllersInput(key, joy);
    assert(pad.GetAccelerate() == 255);
    pad.NewState = pad.ReconcileTwoControllersInput(idle, joy);
    assert(pad.GetAccelerate() > 0 && pad.GetAccelerate() < 255);
    pad.disabled = true;
    assert(pad.GetAccelerate() == 0 && pad.GetBrake() == 0);
    pad.disabled = false; pad.standard = false;
    pad.NewState.Cross = 255; pad.NewState.Square = 255;
    assert(pad.GetAccelerate() == 255 && pad.GetBrake() == 255);
    pad.standard = true;
    memset(pad.CheatString, ' ', sizeof(pad.CheatString));
    joy = {}; joy.RightShoulder2 = 30; Frame(pad, joy);
    assert(pad.CheatString[0] == ' ');
    joy.RightShoulder2 = 31; Frame(pad, joy);
    assert(pad.CheatString[0] == '4' && pad.CheatString[1] == ' ');
    joy.RightShoulder2 = 200; Frame(pad, joy);
    assert(pad.CheatString[1] == ' ');
    replay = true; joy.Cross = 255; Frame(pad, joy);
    assert(pad.CheatString[0] == '4'); replay = false;
    // Gameplay/keyboard state alone is not a physical cheat input.
    Frame(pad, {}); pad.NewState.Triangle = 255; pad.DoCheats(0);
    assert(pad.CheatString[0] == '4');
'''
fixtures = [('3414LDRULDRU', 'WeaponCheat1()'), ('3414LDRULDDL', 'WeaponCheat2()'),
            ('3414LDRULDDD', 'WeaponCheat3()'), ('341CLDRULDRU','HealthCheat()'),
            ('341XLDRULDRU','ArmourCheat()'), ('33C4UDUDUD','WantedLevelDownCheat()'),
            ('CC1CCC123TCT','VehicleCheat(MI_RHINO)'), ('C2LX31X1','ChangePlayerModel("igbuddy")')]
if not vc:
    fixtures = [('4414LDRULDRU', 'WeaponCheat()'), ('4413LDRULDRU','HealthCheat()'),
                ('4412LDRULDRU','ArmourCheat()'), ('4411LDRULDRU','MoneyCheat()'),
                ('4414UDUDUD','WantedLevelDownCheat()'), ('CCCCCC321TCT','TankCheat()')]
for sequence, action in fixtures:
    cpp += f'Code(pad, {json.dumps(sequence)}, {json.dumps(action)});\n'
if vc:
    cpp = '#include "PlayerHealth.h"\n'+cpp
    cpp += '''
    assert(RestorePlayerMaxHealth(250) == 100);
    assert(RestorePlayerMaxHealth(255) == 100);
    assert(RestorePlayerMaxHealth(49) == 150);
    assert(RestorePlayerMaxHealth(99) == 200);
    for (int h : {100, 150, 200, 175}) assert(RestorePlayerMaxHealth(h) == h);
'''
cpp += 'std::cout << "Pad input regression tests passed\\n";\n}\n'
with tempfile.TemporaryDirectory(prefix='pad-input-') as temp:
    temp = Path(temp)
    cpp_path = temp/'test.cpp'
    cpp_path.write_text(cpp, encoding='utf-8')
    exe = temp/('test.exe' if os.name == 'nt' else 'test')
    cxx = os.environ.get('CXX', 'cl' if os.name == 'nt' else 'c++')
    if Path(cxx).stem.lower() == 'cl':
        args = [cxx, '/nologo', '/EHsc', '/std:c++17', '/I'+str(root/'src/core'), str(cpp_path), '/Fe:'+str(exe)]
    else:
        args = [cxx, '-std=c++17', '-I'+str(root/'src/core'), str(cpp_path), '-o', str(exe)]
    subprocess.run(args, cwd=temp, check=True)
    subprocess.run([str(exe)], check=True)
