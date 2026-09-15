# SilentPatch III 第二轮源码核对 / Second source review

参考：官方 [SilentPatch dev](https://github.com/CookiePLMonster/SilentPatch/tree/74976544adc196fa664831ddc59778e684cf6af0)，提交 `74976544adc196fa664831ddc59778e684cf6af0`；核对日期 2026-09-15。
最新发布为 2026 Hotfix #1（III Build 10.1）。本轮以官方源码与本地实际启用条件核对，不根据“原项目 2021 年停止开发”推断缺失项。

上一批 6 类修复已经用户测试通过，上传为 `fa1ba138`。本轮以该提交为基线；用户要求不再测试并直接上传本轮改动。

## 新增修复 / New native adaptations

| 问题 | 本轮修复 | 来源 |
|---|---|---|
| NPC 狙击枪沿用玩家镜头开火路径 | NPC 使用武器发射点的即时命中路径；玩家瞄准镜路径不变。只修正原版 `WEAPON_sniper` 的 0/10/3 动画时序为 0/99/14，保留自定义数据 | `NPCFireSniperFix` |
| NPC 无 seek target 时火箭从身体中心生成 | 保留 NPC 朝向，将生成位置改为调用方提供的发射点；有目标和玩家分支不变 | `NPCFireRPGFix` |
| 东西向路障车辆旋转错误 | 对 `ObjectEastWest` 路段增加 -90° 朝向修正；保持车辆平移和随机数调用数量 | `RoadBlocksHeadingFix` |
| 车辆 extras 未设置环境反射 | 为已从 clump 拆出的附加部件设置材质反射；启用扩展管线时也为它们附加车辆管线，例如 Stinger 车顶 | `CVehicleModelInfo::SetEnvironmentMap_ExtraComps` |

All changes are under `FIX_BUGS`. The vehicle-extra change respects existing material specularity and enabled rendering backends; it does not bundle SilentPatch's vehicle-specific INI overrides or model replacements. No save-format change and no extra assets/ASI are needed.

## 已有或等效，因此未重复改动 / Present or equivalent

| 项目 | 当前 re3 证据与差异 |
|---|---|
| Speeder 坐姿 | `Vehicle.h::GetDriverAnim` 对 Speeder 返回 `ANIM_STD_CAR_SIT`；`PedAI.cpp` 上船与 `Ped.cpp` 设置驾驶员路径已调用它 |
| 浅水车辆阻力 | `Automobile.cpp::ProcessBuoyancy` 已有每个涉水车轮的 -0.003 水平阻力；当前 `config.h` 启用 `GTA_PS2_STUFF` |
| 直升机搜索状态错误落入下一分支 | `Heli.cpp::ProcessControl` 已在 `FIX_BUGS` 下阻止 HOVER 分支落入 CHASE |
| 直升机扬尘高度 | `Heli.cpp::PreRender` 每个点查询地形并在 `FIX_BUGS` 下立即使用其高度；不同于 SilentPatch 轮换缓存，但已避免旧槽位高度错配。不以恢复缓存为由重复修改 |
| 路人躲车方向 | `Ped.cpp::SetEvasiveDive` 已有按来车方向选择侧向躲避的 `FIX_BUGS` 实现 |
| 司机遇袭反应 | `Vehicle.cpp` 已按 `MYRAND_MAX` 缩放阈值，三个分支均可到达；本地步行逃跑阈值 70%，SilentPatch 为 75%，不将概率偏好差异当作缺失修复 |
| 船只存车库崩溃 | `Garages.cpp::CStoredCar::RestoreCar` 已按模型类型构造 `CBoat` |
| 警车追逐 NPC 时因玩家无通缉而放弃 | `CarAI.cpp::MISSION_RAMCAR_CLOSE` 已先判断目标是否为玩家车辆 |
| 免入狱奖励保留武器 | `GameLogic.cpp` 的免费出狱分支不调用 `ClearWeapons` |
| 过场上下黑边缩放 | `Camera.cpp::DrawBordersForWideScreen` 对边缘 8 像素偏移使用 `SCREEN_SCALE_Y`；未声称覆盖所有其他淡出绘制路径 |
| 矩形玻璃、吸引点概率、16 位随机数 | 上一轮已确认存在，见 [首轮记录](SILENTPATCH_PORT.md) |

## 仍需后续专门核对 / Not claimed complete

帮派生成阵型、FBI Kuruma 配色及保险杠材质、脱落肢体 LOD、地图阴影旋转、脚本贴图过滤/缩放、部分文本布局和音频细节仍需逐项检查。此表不是整个 SilentPatch 的完成清单。原版 EXE 地址挂钩、CD/DirectPlay/外部 ASI 兼容修复不能作为 re3 的直接补丁套用。

## Validation

- Windows x64 librw D3D9/OpenAL Release build/link passed.
- 80 checks execute extracted production snippets: NPC/player sniper routing, stock/custom animation timing, NPC projectile origin, both road orientations and random branches, and zero through six vehicle extras including cached environment setup.
- These are isolated checks with engine doubles, not full NPC combat, collision or visual gameplay testing.
- Test suggestions: scripted NPC sniper/RPG firing; roadblocks on differently oriented streets; Stinger with its roof extra in the selected reflection mode. Recheck the player's sniper scope and existing custom weapon data.

Credits: Silent and SilentPatch contributors; [MIT notice](SilentPatch-LICENSE.txt). Adaptations use native re3 source paths, not binary hooks.
