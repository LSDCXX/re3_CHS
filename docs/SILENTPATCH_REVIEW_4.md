# HUD、光效、阴影与大地图 / HUD, effects, shadows and menu map

参考 SilentPatch 源码版本 `74976544adc196fa664831ddc59778e684cf6af0`，包括 `SubtitleRadarCutoutFix`、`ConsoleUIPlacements`、`CoronaFlaresScaling` 和 `CastShadowEntityFix`。许可证见 [SilentPatch-LICENSE.txt](SilentPatch-LICENSE.txt)。

## 本轮修改 / Changes

- `VC_STYLE_SUBTITLES`：游戏中字幕避让完整雷达宽度，并上移一行；过场宽屏字幕居中，恢复对称边距。适用于普通字幕和 Xbox 描边字幕，保留中文字体选择。
- `CONSOLE_BOTTOM_TEXT_PLACEMENTS`：仅恢复底部文字的主机高度，地名 61、车辆名 81、字幕 83、死亡/被捕 122；不改变整个 HUD 样式。两个宏在 `config.h` 中独立启用，可分别关闭。
- 在 `FIX_BUGS` 下按屏幕尺寸缩放 lens flare，不重复缩放普通 corona。
- 在 `FIX_BUGS` 下用完整实体矩阵转换投影顶点，修正倾斜地图物体上的阴影/灯光（weird rock）。
- 大地图取消岛屿进度遮罩与常驻服务点的进度过滤，不修改任务进度、桥梁或游戏世界解锁状态。
- 地图最小尺寸覆盖整个视口；缩放、鼠标拖动、摇杆平移、回到玩家及分辨率切换统一限制地图边界，防止露出地图图集外的黑色底色。贴图本身的海水/黑色区域不做替换。

Gameplay subtitles now reserve the full radar width and move up one line; widescreen cutscene subtitles are centered. Separate enabled configuration macros control subtitle layout and console bottom-text heights, including Xbox outlined subtitles. Lens flares scale with resolution, and projected shadow vertices use the full entity transform. The map shows all islands and configured service markers regardless of progress, with zoom/pan constrained to the atlas bounds. World progression and map texture artwork are unchanged.

## 已有实现 / Already present

- `Population.cpp` 已先计算帮派成员位置，再调用 `IsPositionClearForPed`；已有 1–4 人生成逻辑。`FIX_BUGS` 排除了使成员冻结的 `PED_UNKNOWN` 分支，不重复修改。
- `Automobile.cpp::ProcessBuoyancy` 已在 `GTA_PS2_STUFF` 下逐个涉水车轮施加 `mass * speed * -0.003 * timestep` 水平阻力；当前配置已开启。无需添加另一个速度衰减，避免重复施力。

Gang formation placement/clearance ordering and the anti-freeze guard are already present. Shallow-water wheel resistance is already enabled through `GTA_PS2_STUFF`; no duplicate force is added.

## 验证 / Validation

VS 2026 / v145 完整 Release 编译通过：64 位 D3D9/OpenAL 与 32 位 D3D9/Miles。差异格式检查通过。未运行游戏验证字幕、灯光及地图显示效果；Xbox 字幕可选分支未经单独编译。
Full Release builds passed for 64-bit D3D9/OpenAL and 32-bit D3D9/Miles, plus diff whitespace checks. Visual behavior has not been verified in game; the optional Xbox-subtitle configuration was not separately built.
