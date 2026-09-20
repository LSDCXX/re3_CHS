# 奖励、交通与帮派修复 / Reward, traffic and gang fixes

参考 SilentPatch `74976544adc196fa664831ddc59778e684cf6af0` 的 `SilentPatch/Common.cpp` 和 `SilentPatchIII/SilentPatchIII.cpp`。许可证见 [SilentPatch-LICENSE.txt](SilentPatch-LICENSE.txt)。以下改动由 `FIX_BUGS` 启用。

## 一次免入狱奖励 / Get out of jail free

原有奖励分支不扣钱、不清空武器，但逮捕复活流程末尾又无条件调用 `ClearWeapons`，导致武器仍被清除。现禁用这次重复调用；无奖励时仍在原来的 else 分支扣钱并清空武器，奖励仍只消耗一次。死亡流程不受影响。

This corrects the earlier audit: the reward branch alone was correct, but a later unconditional weapon clear defeated it. Remove that duplicate clear while preserving ordinary arrest penalties, one-time reward consumption and death behavior.

## 交通车辆开灯 / Traffic headlights

雾天、湿路随机阈值按 `MYRAND_MAX` 缩放：15 位随机数使用 25000，16 位使用 50000。当前启用 `USE_PS2_RAND`，本来已有正确的 16 位比例；新逻辑同时覆盖关闭此选项的构建，不会把当前配置的开灯概率误改一倍。时间段开灯规则不变。

Scale the weather-driven switching threshold to the RNG range: 25000 for 15-bit, 50000 for 16-bit. The currently enabled PS2 RNG retains its existing correct probability; PC RNG builds now match it. Time-of-day rules remain unchanged.

## 单行道右转 / Right turns from one-way roads

`CCarCtrl::PickNextNodeRandomly` 已在 `FIX_BUGS` 下正确按当前节点选择左右车道数，符合 Nick007J 的反向条件修复，因此不重复修改。

Already present: the `FIX_BUGS` branch selects the correct lane count for the current route direction.

## FBI 配色 / FBI colours

取消生成追捕车辆时将 FBI 车辆两种颜色强制设为 0 的覆盖，使用车辆模型原本从 `carcols.dat` 选择的颜色。原版资源因此恢复灰色；自定义配色继续生效。未修改模型材质或捆绑新的车辆资源。

Remove the forced black colour override when spawning an FBI pursuit vehicle. Use its model-selected carcols colours, including custom data. No material or model replacement is bundled.

## 帮派刷新 / Gang spawning

纠正之前“编队已完整实现”的结论：防冻结保护已有，但 PC 围绕领队随机散布仍存在。本次按 SilentPatch 的实际路径算法，保存原始生成位置；前 N-1 个成员在生成道路节点间重新选点、找地面并检查新位置；最后一个成员使用原始位置加站立高度，保留领队跟随引用。避免循环叠加 Z 高度，并去除 PC 圆圈偏移。原有距离、碰撞和防冻结检查继续生效。

默认开启队伍生成；可在 `re3.ini` 中设置并由游戏保存：

```ini
[Gameplay]
GangFormations=1
```

设为 0 时每次帮派生成请求只生成一人，取消 50% 空生成；开启时保留 SilentPatch 的原组队概率。此为运行配置，不改变存档结构。

Formation mode samples the original pedestrian path segment and checks each final spawn position, preserving leader references. The last member uses the saved original position. Disabling formations generates one member per request without the original 50% empty-spawn chance. The setting is persisted in `re3.ini`, without save-layout changes.

## 验证 / Validation

源码审阅、差异格式检查，64 位 OpenAL 和 32 位 Miles Release 构建。游戏内奖励、帮派行为及视觉效果仍需实测。
Source review, diff-format checks and Release builds for 64-bit OpenAL and 32-bit Miles; gameplay and visual verification remains pending.
