# SilentPatch III 第三轮移植 / Third native adaptation

来源 / Source: [SilentPatch III dev](https://github.com/CookiePLMonster/SilentPatch/blob/74976544adc196fa664831ddc59778e684cf6af0/SilentPatchIII/SilentPatchIII.cpp), revision `74976544adc196fa664831ddc59778e684cf6af0`.

本轮五项均置于 `FIX_BUGS` 下，无需额外游戏资源，不改变存档格式。
All five changes are guarded by `FIX_BUGS`, require no additional assets and preserve the save format.

| 修复 / Fix | 实现 / Implementation |
|---|---|
| 拾取物槽位复用泄漏 / Pickup slot reuse leak | 替换临时拾取物前移除并释放旧的世界对象。Remove and delete the previous world object before reusing its pickup slot. |
| 清除任务音频后仍播放 / Mission audio continues after clearing | PC 停止流式任务音频，PS2 停止专用任务音频通道，再清理状态。Stop the mission stream on PC or the dedicated mission channel on PS2 before resetting state. |
| 音效反射污染后续声源 / Reflections alter subsequent sound sources | 生成反射后恢复原始声源位置与距离。Restore source position and distance after reflection generation. |
| Decoy 路障车辆优先级 / Decoy roadblock priority | 优先选择 SWAT，再 FBI、Army，保留模型未加载时的警车回退。Prefer SWAT, then FBI, then Army; retain the unloaded-model fallback. |
| 车辆掩体位置 / Vehicle cover position | 以前轮中点为基准，向远离威胁方向偏移 1.5；仅选择具有两个前轮节点的汽车。Use the front-wheel midpoint, offset 1.5 away from the threat; require an automobile with both front-wheel nodes. |

## 撤回与重做 / Revert and correction

首次提交 `2d0a5e72` 在 PC 中引用了仅 PS2 定义的 `CHANNEL_MISSION_AUDIO`，导致 C2065 编译错误，已通过 `2f12630` 撤回。本次按 `ProcessMissionAudio` 的平台分支重新实现，不为 PC 添加虚假的通道编号。
The first revision referenced the PS2-only mission channel on PC, causing C2065. It was reverted in `2f12630`. The replacement follows the existing playback backend's platform branches.

## 车辆灯光位置 / Vehicle corona positions

按上述 SilentPatch 版本的 `SilentPatchIII.cpp`、`SilentPatch/Common.cpp` 和 `SilentPatch/SVF.cpp` 移植，使用车辆矩阵将局部坐标转换到世界坐标，均位于 `FIX_BUGS` 下。
Ported from the referenced SilentPatch revision, with local offsets transformed by the vehicle matrix, guarded by `FIX_BUGS`.

| 车型 / Model | 局部坐标 / Local position (x, y, z) |
|---|---|
| Firetruck 消防车 | (±0.95, 3.2, 1.4) |
| Ambulance 救护车 | (±0.7, 0.65, 1.55) |
| Enforcer 警用厢车 | (±0.6, 1.05, 1.4) |
| Taxi 顶灯 | (0, -0.25, 0.9) |
| Police chopper 探照灯光晕 | (0, 3.0, -1.25) |

普通 Police、Cabbie、Borgnine 坐标保持不变；SilentPatch III 未给这些车型提供本项位置修正。坐标针对原版模型，自定义车辆模型可能仍需自行调整。
Police, Cabbie and Borgnine retain their original positions, matching SilentPatch III's scope. These offsets target stock models; replacement models may need different offsets.

## 验证 / Validation

2026-09-20：完整 Release 编译通过（VS 2026 / v145）：`win-amd64-librw_d3d9-oal` 和 `win-x86-librw_d3d9-mss`；`git diff --check` 通过。未运行游戏测试，未编译 PS2 目标。
Full Release builds passed for 64-bit OpenAL and 32-bit Miles with VS 2026/v145. Diff whitespace checks passed. In-game behavior and the PS2 target were not tested.

SilentPatch attribution and MIT license: [SilentPatch-LICENSE.txt](SilentPatch-LICENSE.txt).
