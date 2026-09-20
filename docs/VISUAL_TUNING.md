# 武器图标修复与距离设置 / Weapon icons and distance tuning

## 武器图标 / Weapon icons

源码核对确认两项缺失，现已在 `FIX_BUGS` 下修复：将错误的三个 `0.015` UV 值归零；仅在绘制武器图标期间使用双线性过滤，并恢复原过滤状态。RenderWare 状态使用 `uint32` 接收，兼容 32/64 位。

Both issues were present. Under `FIX_BUGS`, the three erroneous UV offsets are zeroed and the weapon icon uses linear filtering with the previous state restored afterwards.

来源：[UV 修复](https://github.com/CookiePLMonster/SilentPatch/commit/f91651181271db77082c8d33f09f4f01d821ed3c)、[过滤修复](https://github.com/CookiePLMonster/SilentPatch/commit/a1dd33fd0447b29144af0b52d264b61c5055b85d)。许可见 [SilentPatch-LICENSE.txt](SilentPatch-LICENSE.txt)。

## 现代 PC 距离预设 / Modern PC distance preset

按用户引用聊天中选定的增强值，参考 [III MixSets 功能分类](https://www.mixmods.com.br/2021/08/iii-mixsets/) 在 re3 原生实现，并非加载 MixSets ASI，也不声称这些数值都是 MixSets III 默认值。常量统一放在 `src/core/VisualTuning.h`，修改后重新编译生效。

Native re3 implementation of the requested preset, inspired by MixSets' distance controls. This does not require its ASI or claim these are its default settings. Edit `VisualTuning.h` and rebuild to tune the values.

| 项目 / Setting | 本地原值 / Previous | 新值 / New |
|---|---:|---:|
| 普通车辆高模 / Vehicle high detail | 70 | 200 |
| 普通车辆低模可见范围 / Low-detail visibility | 90 | 250 |
| 车辆 fade 参数 / Fade parameter | 100 | 260 |
| 大型车辆高模、低模 / Big vehicle high/low detail | 60 / 150 | 200 / 250 |
| 常规回收屏内、屏外基础范围 / Despawn on/off-screen | 130 / 50* | 250 / 150 |
| 另一条屏外交通回收路径 / Additional off-screen traffic cleanup | 25 | 150 |
| 汽车阴影 / Car shadow | 18 | 300 |
| 人物阴影实际截止距离 / Actual ped shadow cutoff | 13 | 300 |
| 交通灯光晕 / Traffic light corona | 50 | 300 |
| 交通灯地面光影 / Traffic light ground projection | 40 | 300 |

*原 `EXTENDED_OFFSCREEN_DESPAWN_RANGE` 配置会把常规回收路径统一为 130；现在使用显式的两档预设。保留原有镜头观察方向、停放车辆和紧急车辆等延长范围条件、任务/锁定车辆及车库保护；原相机距离倍率和 extended-range 倍率仍适用。人物阴影按距离后半段淡出，避免仅改常量导致截止距离减半或强度变负。

The explicit preset supersedes the old unified off-screen range option. Existing special-camera/parked/emergency range conditions, mission/locked vehicle and garage protections remain. Camera/range multipliers still apply. Ped shadows fade over the outer half of their actual range.

这些值是各子系统上限，不会强制生成 300 范围内所有行人/车辆或加载全部地图物体。阴影依赖实体存在、可见和模型已加载；交通灯同样依赖实体预渲染。没有调整交通/行人密度、地图 LOD、天气雾距、streaming 内存或阴影池容量。更长保留及绘制范围会增加资源占用，并可能影响交通回收节奏。

These are subsystem limits, not a guarantee that all entities are spawned/rendered out to 300 units. Entity visibility, streaming, fog and existing pools still apply. Density, map LOD, streaming memory and pool capacities are unchanged. Longer draw/retention ranges increase resource use and can affect traffic recycling.

## 验证 / Validation

64 位 D3D9/OpenAL、32 位 D3D9/Miles Release 编译与链接均通过（VS 2026 / v145）；差异格式检查通过。尚未验证游戏内视觉效果、交通密度变化或性能。
Both 64-bit D3D9/OpenAL and 32-bit D3D9/Miles Release builds compile and link successfully. Diff whitespace checks pass. In-game visuals, traffic behavior and performance have not been measured.
