# PS2 手柄作弊码与模拟油门

## 使用方法

- 使用 XInput 兼容手柄；PlayStation 手柄经 XInput 映射后也可使用。
- 在游戏过程中依次按下作弊码，每个按键按下后松开。方向使用十字键。
- 按键按手柄实际位置识别，不随 Standard / Classic 操作布局改变。
- Xbox 对应关系：△ = Y、○ = B、× = A、□ = X、L1 = LB、L2 = LT、R1 = RB、R2 = RT。
- LT / RT 超过 30/255 才计为按下，按住不会重复录入；暂停菜单内不录入，进入暂停会清空未完成的序列。
- 在 Standard Controls 下，RT 为模拟油门、LT 为模拟刹车／倒车。前 30/255 行程为死区，剩余行程线性映射到 0～255；半按不再变成全油门。键盘按键仍输出全量，Classic 布局保持原有油门按键。
- 使用现有游戏内作弊效果与提示；这不是 ASI 插件，无需另装 GInput。

## 按键表

参考 [GInput 官方说明](https://silentsblog.com/mods/gta-iii/)及其下载包中的 `docs/cheat_list_ps3.html`。以下为本分支接入的序列。

| 效果 | 依次按下 |
| --- | --- |
| WeaponCheat | R2 R2 L1 R2 ← ↓ → ↑ ← ↓ → ↑ |
| MoneyCheat | R2 R2 L1 L1 ← ↓ → ↑ ← ↓ → ↑ |
| ArmourCheat | R2 R2 L1 L2 ← ↓ → ↑ ← ↓ → ↑ |
| HealthCheat | R2 R2 L1 R1 ← ↓ → ↑ ← ↓ → ↑ |
| WantedLevelUpCheat | R2 R2 L1 R2 ← → ← → ← → |
| WantedLevelDownCheat | R2 R2 L1 R2 ↑ ↓ ↑ ↓ ↑ ↓ |
| SunnyWeatherCheat | L1 L2 R1 R2 R2 R1 L2 △ |
| CloudyWeatherCheat | L1 L2 R1 R2 R2 R1 L2 □ |
| RainyWeatherCheat | L1 L2 R1 R2 R2 R1 L2 ○ |
| FoggyWeatherCheat | L1 L2 R1 R2 R2 R1 L2 × |
| TankCheat | ○ ○ ○ ○ ○ ○ R1 L2 L1 △ ○ △ |
| FastWeatherCheat | ○ ○ ○ □ □ □ □ □ L1 △ ○ △ |
| BlowUpCarsCheat | L2 R2 L1 R1 L2 R2 △ □ ○ △ L2 L1 |
| ChangePlayerCheat | → ↓ ← ↑ L1 L2 ↑ ← ↓ → |
| MayhemCheat | ↓ ↑ ← ↑ × R1 R2 L2 L1 |
| EverybodyAttacksPlayerCheat | ↓ ↑ ← ↑ × R1 R2 L1 L2 |
| WeaponsForAllCheat | R2 R1 △ × L2 L1 ↑ ↓ |
| FastTimeCheat | △ ↑ → ↓ □ L1 L2 |
| SlowTimeCheat | △ ↑ → ↓ □ R1 R2 |
| OnlyRenderWheelsCheat | L1 L1 □ R2 △ L1 △ |
| ChittyChittyBangBangCheat | → R2 ○ R1 L2 ↓ L1 R1 |
| StrongGripCheat | R1 L1 R2 L1 ← R1 R1 △ |
| NastyLimbsCheat | □ L1 ○ ↓ L1 R1 △ → L1 × |
| KangarooCheat | R1 □ ← → R1 ○ ↑ ↓ L1 × |
| CPed::SwitchDebugDisplay | ↓ ↑ × R1 L1 ↓ ↑ L1 R1 |
| AllCarsHeliCheat | △ △ L1 → R1 ← ○ ○ ↑ |
| AltDodoCheat | R1 L1 × × L1 R1 ↑ ↑ ↓ |

本表沿用 GTA III 原有 PS2 序列；袋鼠跳、直升机汽车、特殊 Dodo 等扩展受对应编译开关控制，调试项仅限非 MASTER 构建。

## 实现与验证

- 合并键盘、鼠标、手柄输入时保留 L2/R2 压力值，避免被按钮逻辑提升为 255。
- 作弊码使用合并前的手柄状态；暂停、新游戏清理输入状态，回放不触发手柄作弊。
- `tests/test_pad_input.py` 从实际 `Pad.cpp` 提取输入合并、油门、刹车、边沿识别和序列匹配方法编译运行。作弊效果使用桩函数，测试不替代游戏场景验证。
- 在 Visual Studio x64 开发者命令行运行 `python tests/test_pad_input.py`；其他平台可设置 `CXX` 使用 C++17 编译器。
- 本次通过 Release x64 D3D9/OpenAL 修改文件编译及完整程序链接，以及输入与血量回归测试。MSBuild 文件跟踪组件在当前受限环境报权限错误，使用已有构建参数直接调用 MSVC 编译器／链接器完成增量构建。
- 尚未进行实体手柄驾驶、全部作弊效果、旧存档死亡复活的游戏内实测。
