# SilentPatch III source comparison and native fixes

Reference checked on 2026-09-15: official [CookiePLMonster/SilentPatch](https://github.com/CookiePLMonster/SilentPatch), `dev` at `74976544adc196fa664831ddc59778e684cf6af0` (2026-09-01).
This is a selected source-level backport, not the ASI plugin or a claim that every SilentPatch fix is present. The existing re3 camera, input, map and Chinese rendering integrations are retained.

## 本次移植 / Changes

| 修复 / Fix | re3 source | SilentPatch III reference |
|---|---|---|
| 救护车奖励不再降低超过 100 的血量 / Ambulance bonus preserves health above 100 | `CVehicle::SetDriver` | `AmbulanceHealthBonusFix` |
| 灭火不再修复报废车血量和发动机 / Extinguishing fire preserves wrecked health and engine damage | `CVehicle::ExtinguishCarFire` | `ExtinguishCarFireFix`; native engine-status guard also added |
| 旋转玻璃在模型坐标求边界后变换，碎片不再错位 / Transform pane bounds and axes from model space | `CGlass::WindowRespondsToCollision` | `GlassPanesRotationFix` |
| 彩色文字标记保留淡出透明度 / Colour tokens preserve fade alpha | Both `CFont::ParseToken` variants | `TokenColorFix` |
| 任务结束、新游戏和载入前初始化时重置进车与威胁范围倍率 / Reset enter-car and threat range multipliers on cleanup and script initialization | `CMissionCleanup::Process`, `CTheScripts::Init` | `GameVariablesToReset`, `GameVariablesToMissionCleanup` |
| 恢复 brakelights 节点；缺失时回退尾灯位置 / Restore brake/reverse-light dummy with tail-light fallback | Vehicle model node table and lights-off brake/reverse rendering | `BrakelightsDummy` |

All changes are guarded by `FIX_BUGS`. The lamp change follows SilentPatch's lights-off branch; it does not replace every nighttime tail-light effect or add a separate reverse-light dummy. No save-layout changes or additional game assets are required. Existing PC models without `brakelights` continue using `taillights`.

## 已有修复 / Already present

- Rectangular glass shard scaling already uses `upLen` under `FIX_BUGS` in `GeneratePanesForWindow`.
- Malformed attractor probabilities already clamp to 0–255 under `FIX_BUGS` in `CFileLoader::Load2dEffect`.
- The active `USE_PS2_RAND` configuration already uses a 65535 random-number maximum.
- Script range opcodes already decode float parameters under `FIX_BUGS`; this patch adds the missing resets without changing their existing integer storage semantics.

These were not ported again. Original-EXE hooks for CD checks, DirectPlay, executable addresses and binary compatibility are not applicable as drop-in patches to re3. Vehicle extras, Speeder seating, NPC sniper/RPG AI, shallow-water physics and helicopter behavior need separate source and gameplay review; they are not claimed as fixed by this patch.

## 验证 / Validation

- Windows x64 librw D3D9/OpenAL Release compile/link passed. Existing pointer-size cast warnings remain.
- 81 isolated checks extract the modified production code for ambulance health, extinguishing live/wrecked vehicles, color alpha including adjacent tokens, rotated/tilted glass, brake dummy fallback and script reset assignments.
- Patch whitespace checks passed. The user subsequently reported completing gameplay testing successfully; the specific scenarios covered were not enumerated.
- The earlier controller-map cursor fix was user-tested and uploaded separately as `68d0e2e9`. The user approved this first SilentPatch batch for upload after testing.

Credits: Adrian Zdanowicz (Silent) and SilentPatch contributors. See [the upstream MIT notice](SilentPatch-LICENSE.txt). The native adaptations use re3 types and functions instead of executable-address hooks. The rest of re3 retains its existing licensing notices.
