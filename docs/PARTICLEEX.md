# ParticleEx 原生移植 / Native port

## 使用方法

默认保持本分支原有 **PC 粒子**。将仓库 `gamefiles/ParticleEx` 文件夹复制到游戏根目录，与 `re3.exe` 放在同一级；不要把它放进 `models`。

退出游戏后，在游戏使用的 `re3.ini` 中添加或修改：

```ini
[ParticleEx]
System=0
```

| System | 粒子系统 |
| --- | --- |
| `0` | PC，默认 |
| `1` | PS2 |
| `2` | Xbox |

修改后重新启动游戏。此编号是本分支的配置约定，不是上游 ASI 的内部枚举。无需安装 `IIIParticleEx.asi`，也不读取上游 `IIIParticleEx.ini`。

PC 模式不需要新增资源。选中的 PS2／Xbox 配置文件缺失、格式不完整，或 TXD 加载失败、缺少所需纹理时，启动回退到 PC，并在日志中提示。缺少原游戏本身的 PC 资源不在此回退范围内。

## 移植内容

- Fire_Head 的 PS2、Xbox 粒子创建、更新、渲染、爆炸喷流和 Yardie 车门烟雾实现；保持原有 PC 实现。
- 按名称对应的 Xbox 粒子编号映射，支持 Xbox 特有的移动车辆火焰及 45 帧火焰纹理。
- Xbox 地面／人物／车辆火焰、喷火器、场景火焰、黑烟及消防栓；燃烧瓶火焰以实际火源为中心分布。
- 将 re3 已有的 PS2 水花分支接入运行时选择：车辆／行人落水、水洼、雨天脚步与轮胎、船只水花，以及场景蒸汽和火焰的生成频率。
- PS2／Xbox 使用左前翼子板位置生成车辆爆炸喷流；PC 保持原分支行为。浅水阻力和爆炸焦痕修复在此分支中已经存在，不重复施加。
- 读取存档后按当前模式调整场景粒子发射器，不改变存档结构。切换发生在下次启动，不在运行中销毁仍被粒子对象引用的粒子池。
- 两套新增系统的动画、淡出和旋转以 30 Hz 更新，避免高帧率加快这些计数器；物理步长与此更新周期对应。PC 原有计时路径保持不变。
- 配置采用有界、完整条目读取，失败时保留当前配置；纹理预检查；绘制时限制帧索引，处理上游部分配置帧数大于实际纹理数组的问题。

这是针对 re3 源码的适配，支持原生粒子对象与 32／64 位指针；不包含原 EXE 地址补丁、ASI 检测、外部 Waterdrops 插件钩子、调试编辑器及 `.pobj` 导入导出工具。保留 1000 个粒子的池容量；未提供上游所有实验性开关。PS2／Xbox 各自使用随附配置与纹理，原有 PC 的 `particle.cfg`／`particle.txd` 不被覆盖。

## 来源

原作者：[Fire_Head / ParticleEx](https://github.com/Fire-Head/ParticleEx)。本次按用户提供的 [enborballer/ParticleEx](https://github.com/enborballer/ParticleEx) 源码快照 `62142eaccba117961f02a621ff4781955b894683` 移植 `IIIParticleRE`，资源取自该快照的 `Release/III/ParticleEx/PS2` 和 `XBOX`。保留来源署名；该快照未附独立 LICENSE 文件，不为上游代码和资源额外声明许可。

## 验证范围

使用 C++17 编译独立配置读取检查，覆盖有效配置、空文件、错误名称、超长行、截断、额外条目、缺文件及无末尾换行。核对 PS2 的 68 个配置条目和 Xbox 的 69 个配置条目及所需纹理。Windows Release 的 x64 D3D9/OpenAL 与 x86 D3D9/Miles 构建均需通过。

尚未在真实游戏中验证画面。建议分别检查：雨天跑步和轮胎水花、汽车与行人落水、船只行驶、燃烧瓶、喷火器、行驶／静止车辆起火和爆炸、Yardie 车门、读取存档及任务重试；同时比较 30／60 FPS。

---

## English

The existing **PC particle system remains the default**. Copy `gamefiles/ParticleEx` beside `re3.exe`, then set `[ParticleEx] System=0` (PC), `1` (PS2), or `2` (Xbox) in `re3.ini` while the game is closed. Restart to apply. These numbers are specific to this port. The original ASI and its INI are not required.

The selected console system falls back to PC if its configuration is missing/invalid or its texture dictionary cannot be loaded or lacks required textures. PC still needs the original game resources.

This native adaptation includes the two console particle engines, Xbox animated and moving flames, flamethrower effects, centred molotov fire, emitter differences and re3's PS2 water-effect branches. Saved emitters are normalized to the selected system without changing the save format. Console fade/animation/rotation counters tick at 30 Hz; the existing PC timing is unchanged. Configuration reloads are transactional, and raster indexing is bounded independently of animation lifetime.

The pools retain 1000 particles. EXE hooks, external ASI/Waterdrops integration, developer editors, `.pobj` import/export and all upstream experimental switches are outside this adaptation. Console assets are separate from the original PC assets.

Credit: Fire_Head, using the user-specified enborballer snapshot linked above. No separate upstream license file was present in that snapshot; no new license is asserted for upstream code or assets.

Validation covers a standalone C++17 parser check, resource completeness, x64 D3D9/OpenAL and x86 D3D9/Miles Release builds. Actual gameplay and visual comparisons remain to be tested, especially water, fire, explosions, save loading and 30/60 FPS.
