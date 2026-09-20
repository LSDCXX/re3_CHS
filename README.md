# re3

**简体中文** | [English](README.en.md)

## 本次整合的功能与修复

- **ParticleEx 粒子系统**：保留 PC 为默认，可通过配置选择 PS2／Xbox；包括对应火焰、烟雾和水花，资源缺失时回退 PC。切换与安装见 [ParticleEx 说明](docs/PARTICLEEX.md)。

- **过场动画可动手指**：原生适配 aap/iii_anim，每只手使用独立动画状态；缺少兼容资源时使用原手部。III 的手部资源需另行安装，详见 [资源说明](docs/cutscene-hands-menu-map.md)。
- **全屏大地图**：适配 LSDCXX/MenuMapIII，保留黄色底栏；上方居中图例、右下角黄条上方地名、鼠标悬停名称，调整字号、背景和阴影，修复玩家方向与返回按钮。
- **地图交互**：从暂停菜单进入，取消 M 打开/关闭地图；Esc、返回按钮或 B 返回上一级，Start 恢复游戏。左摇杆移动、LT/RT 缩放、X/方块标点、LB/L1 图例。
- **Classic Axis**：Standard Controls 下支持越肩第三人称瞄准、居中准星与蹲下射击，Free Cam 独立；修复走路偏移、跳跃和车库误显示准星、连续射击/蹲射节奏、先蹲后瞄无法开火、持续瞄准走跑异常及自定义步行键。喷火器站立使用原版动画。
- **GInput 第五套风格手柄布局**：LT 瞄准、RT 开火、A 奔跑、X 跳跃、Y 上车、LS 蹲下；载具 RT 油门、LT 刹车/倒车、A 手刹、B 开火、LB/RB 左右看，同时按向后看。
- **锁定与自由瞄准**：手柄锁定、十字键左右切换目标，目标死亡后选择附近最近的有效敌人；推动右摇杆转自由瞄准，松开 LT 后才重新允许自动锁定。
- **中文手柄图标**：左上角操作提示随设备和设置中的手柄类型显示对应图标，修复中文不显示按键；图标宽度参与换行与对齐。
- **任务重试**：保留并完善已有移动版任务重试实现，加入快照有效性检查、中英文提示和取消操作；防止失败写入使用旧快照，保留任务开始时的快照。

完整配置、修复迭代与验证限制见 [Classic Axis 说明](docs/CLASSIC_AXIS.md)；最新章节取代文档中早期修订的旧行为。本次未系统移植最新版 SilentPatch，也不将未经证实的爆炸音效问题列为已修复。

当前仓库克隆命令：

```sh
git clone --recursive --branch master https://github.com/LSDCXX/re3_CHS.git re3
```

**旧版说明保留在下方**：历史上游下载链接、平台测试范围和 C++ 限制不代表本分支新增代码。新增功能的构建与隔离测试不替代完整游戏测试。

来源与致谢：[iii_anim](https://github.com/aap/iii_anim)、[MenuMapIII](https://github.com/LSDCXX/menu-map-iii)、[原 MenuMap](https://github.com/gennariarmando/menu-map)、[Classic Axis](https://github.com/gennariarmando/classic-axis)、[GInput III](https://silentsblog.com/mods/gta-iii/#ginput)、[ZeptoBST/re3](https://github.com/ZeptoBST/re3)。中文术语参考 [GTAMODX 介绍译文](https://gtamodx.com/mods/BmUYhBABACsQ)。原有署名、截图、链接、配置、构建说明、历史和许可均保留；[未经改写的旧 README](docs/README_LEGACY.md) 可供对照。


---

<img src="https://github.com/hezkore/re3/blob/master/res/images/logo_1024.png?raw=true" alt="re3 logo" width="200">


## 关于这个 re3 分支

本分支修复了作者在原版 re3 中遇到的问题，旧版这些改动仅应用于 GTA III：

* 镜头水平与垂直速度一致。
* 整体降低鼠标速度，使灵敏度可以更细致地调整。
* 粒子不再受帧率影响（原说明注明使用了简易固定更新频率处理）。

## 简介

## 动态中文字库

Windows 下可从已安装字体或本地 TTF/TTC/OTF 文件按需生成中文字形。在 `re3.ini` 中配置：

```ini
[Fonts]
TextRenderer=3
NormalFonts=Microsoft YaHei,SimSun,SimHei,DengXian
NormalBold=1
SlantFontFile=
SlantBold=1
GlyphHeight=56
RareFontFile=C:\Windows\Fonts\msyh.ttc,C:\Windows\Fonts\SimsunExtG.ttf
NormalWeight=700
SlantWeight=700
RareWeight=400
```

`TextRenderer=3` 使用默认 DirectWrite，`2` 使用兼容的 GDI，`1` 保留旧 `MODELS/CHINESE.TXD` 加 `data/Chinese.dat` 路径。动态模式在遇到字符时将字形缓存在纹理图集中，不受静态纹理所带字形数量限制；实际覆盖取决于字体。

加载中文时，`CHINESE.GXT` 引用的常规字形会在加载画面期间预生成，不论字体系统还是 GXT 先加载完成。首次加载稍长，但避免新过场字幕在游戏中触发卡顿。同一字符串内不在 GXT 中的字符共用一次图集锁定／上传。中文 HUD 字形放大 10%；与 reVC 一样，按图集纹理分组，阴影与正文分别批量提交并使用线性过滤。拉丁文字保留原版尺寸与渲染路径。

旧上游仓库提供 GTA III（[master](https://github.com/hezkore/re3/tree/master/)）与 GTA Vice City（[miami](https://github.com/GTAmodding/re3/tree/miami/)）的完整逆向源代码。

原项目曾在 Windows、Linux、macOS、FreeBSD 以及 x86、amd64、ARM、ARM64 上测试运行。图形可使用原版 RenderWare（D3D8），或重新实现的 [librw](https://github.com/aap/librw)（D3D9、OpenGL 2.1+、OpenGL ES 2.0+）；音频可使用 MSS（原版 GTA DLL）或 OpenAL。

项目也被移植到 [Nintendo Switch](https://github.com/AGraber/re3-nx/)、[PlayStation Vita](https://github.com/Rinnegatamante/re3) 和 [Nintendo Wii U](https://github.com/GaryOderNichts/re3-wiiu/)。旧版说明指出尚不能构建 PS2 或 Xbox 版本，有意参与者可联系原开发者。

## 安装

- re3 需要 PC 原版游戏资源才能运行，必须拥有一份正版 GTA III。以下保留旧版商店与构建链接（历史地址）：
- [原游戏商店链接](https://store.steampowered.com/app/12100/Grand_Theft_Auto_III/)。编译 re3，或参考旧版列出的构建包：
  - [Windows D3D9 MSS 32 位](https://nightly.link/GTAmodding/re3/workflows/re3_msvc_x86/master/re3_Release_win-x86-librw_d3d9-mss.zip)
  - [Windows D3D9 64 位](https://nightly.link/GTAmodding/re3/workflows/re3_msvc_amd64/master/re3_Release_win-amd64-librw_d3d9-oal.zip)
  - [Windows OpenGL 64 位](https://nightly.link/GTAmodding/re3/workflows/re3_msvc_amd64/master/re3_Release_win-amd64-librw_gl3_glfw-oal.zip)
  - [Linux 64 位](https://nightly.link/GTAmodding/re3/workflows/build-cmake-conan/master/ubuntu-18.04-gl3.zip)
  - [macOS 64 位 x86-64](https://nightly.link/GTAmodding/re3/workflows/build-cmake-conan/master/macos-latest-gl3.zip)
- 将下载的运行包解压到 GTA III 游戏目录并运行 re3。原说明中的 ZIP 包含可执行文件、更新与额外资源；OpenAL 版本还包含必需 DLL。源码 ZIP 本身不含已编译程序。


## 截图

![re3 2021-02-11 22-57-03-23](https://user-images.githubusercontent.com/1521437/107704085-fbdabd00-6cbc-11eb-8406-8951a80ccb16.png)
![re3 2021-02-11 22-43-44-98](https://user-images.githubusercontent.com/1521437/107703339-cbdeea00-6cbb-11eb-8f0b-07daa105d470.png)
![re3 2021-02-11 22-46-33-76](https://user-images.githubusercontent.com/1521437/107703343-cd101700-6cbb-11eb-9ccd-012cb90524b7.png)
![re3 2021-02-11 22-50-29-54](https://user-images.githubusercontent.com/1521437/107703348-d00b0780-6cbb-11eb-8afd-054249c2b95e.png)


## 改进

我们对原版游戏进行了多项修改和改进，可在 `src/core/config.h` 中配置；部分可在运行时切换，其他需要编译时选择。

* 修复大量大小 Bug。
* 用户文件（存档和设置）存放在 GTA 游戏根目录。
* 设置保存在 `re3.ini` 而不是原版 `gta3.set`。
* 支持带骨骼的行人模型（Xbox 或移动版）。
* PS2 粒子效果。
* 菜单地图。
* 增加调试菜单（Ctrl-M），用于执行或修改各种内容。
* 增加调试摄像机（Ctrl-B 切换）。
* 支持旋转摄像机。
* Windows 支持 XInput 手柄。
* 岛屿间可取消加载画面（菜单中的“地图内存使用量”）。
* 渲染：
  * 宽屏支持，正确缩放 HUD、菜单与 FOV。
  * PS2 MatFX 车辆反射。
  * PS2 Alpha Test，改善透明材质。
  * Xbox 车辆渲染。
  * Xbox 世界光照贴图（需要 Xbox 地图）。
  * Xbox 行人边缘光。
  * Xbox 屏幕雨滴。
  * 更自由的颜色滤镜设置。
* 菜单：更多选项、手柄配置菜单等。
* 可加载其他平台的 DFF 和 TXD，但可能有性能损失。
* ……

## 待办事项

以下为原版 README 的开发方向，不表示本分支已经完成：

* 修复高帧率物理问题。
* 与 PS2 代码对比（工作繁琐，缺乏好用的反编译器）。
* 改善低端设备性能，尤其是 Raspberry Pi 的 OpenGL 图形层；欢迎有经验的开发者参与。
* [PS2 移植](https://github.com/GTAmodding/re3/wiki/PS2-port)。
* Xbox 移植（优先级较低）。
* 逆向剩余未使用与调试函数。
* 将 CodeWarrior 编译结果与原始二进制对比，提高还原准确性（工作繁琐）。

## Mod 支持

模型、纹理、操控参数、脚本等资源修改，在大多数情况下与原版 GTA 的 Mod 制作方式相同。

修改原版程序代码的 DLL/ASI、限制调整器通常不能直接使用。它们的部分功能已在 re3 中实现，例如 SkyGFX、GInput、SilentPatch、Widescreen Fix 的部分功能；其他功能可以通过 `config.h` 提高限制，或重写后直接集成到源码中。对于这种不便，原项目表示歉意。

CLEO 脚本可通过 [CLEO Redux](https://github.com/cleolibrary/CLEO-Redux) 工作。

## 从源代码编译

使用 Premake 时，可将 `GTA_III_RE_DIR` 环境变量指向游戏根目录，通过编译后脚本复制可执行程序。

旧版上游克隆命令：`git clone --recursive https://github.com/GTAmodding/re3.git`。然后 `cd re3` 进入目录。本分支地址请使用本页开头的新命令。

<details><summary>Linux Premake</summary>

Linux 使用 Premake 请参考：[在 Linux 上编译](https://github.com/GTAmodding/re3/wiki/Building-on-Linux)

</details>

<details><summary>Linux Conan</summary>

安装 Python 与 Conan，然后执行旧版构建命令（依赖版本以工程要求为准）：
```
conan export vendor/librw librw/master@
mkdir build
cd build
conan install .. re3/master@ -if build -o re3:audio=openal -o librw:platform=gl3 -o librw:gl3_gfxlib=glfw --build missing -s re3:build_type=RelWithDebInfo -s librw:build_type=RelWithDebInfo
conan build .. -if build -bf build -pf package
```
</details>

<details><summary>MacOS Premake</summary>

macOS 使用 Premake 请参考：[在 macOS 上编译](https://github.com/GTAmodding/re3/wiki/Building-on-MacOS)

</details>

<details><summary>FreeBSD</summary>

FreeBSD 使用 Premake 请参考：[在 FreeBSD 上编译](https://github.com/GTAmodding/re3/wiki/Building-on-FreeBSD)

</details>

<details><summary>Windows</summary>

旧版以 Visual Studio 2015/2017/2019 为例；当前新增代码应使用符合工程要求的工具集：
- 运行根目录对应的 `premake-vsXXXX.cmd`。
- 在 Visual Studio 中打开 `build/re3.sln` 并编译解决方案。

旧文记载微软已停止提供 DX9 SDK 下载，可参考归档版本： https://archive.org/details/dxsdk_jun10

**Windows 选择 OpenAL 时**，请阅读 [在 Windows 上运行 OpenAL 构建](https://github.com/GTAmodding/re3/wiki/Running-OpenAL-build-on-Windows).
</details>

> :information_source: Premake 的 `--with-lto` 选项用于链接时优化（LTO）。

> :information_source: 各种配置开关见 [config.h](https://github.com/GTAmodding/re3/tree/master/src/core/config.h)，建议查阅。

> :information_source: 项目使用自行开发的 RenderWare 替代引擎 [librw](https://github.com/aap/librw/)。它作为 Git 子模块提供，也可以通过 `LIBRW` 环境变量指定自己的 librw 路径。

如有需要，也可使用 CodeWarrior 7 与 `codewarrior/re3.mcp` 工程编译，需要原版 RW33 库和 DX8 SDK。相比 MSVC，此构建方式不稳定，主要用作参考。


## 贡献代码

以下保留原项目的贡献约定：除 Linux／跨平台框架／兼容层代码外，未放在预处理条件（如 `FIX_BUGS`）内的代码，均来自原始二进制的逆向。自定义代码应由预处理条件封装，或属于上述跨平台部分。

原项目接受的 PR 类型：

- 至少在一部 GTA 中存在的新功能；若 III/VC 原本没有，不要求来自反编译。
- 游戏、界面或体验修复；原始游戏代码的 Bug 修复应位于 `FIX_BUGS` 条件下。
- 尚未逆向的平台专用代码及未使用代码。
- 使逆向代码更易理解、更准确，例如更接近原始汇编生成方式。
- 新的跨平台框架／兼容层及其改进。
- 原游戏支持语言的翻译修复。
- 提高代码可维护性的修改。

原项目提供 [代码风格说明](https://github.com/GTAmodding/re3/blob/master/CODING_STYLE.md)，但执行并不严格。

旧版要求“不要使用 C++11 或更新特性”。这是保留的历史要求；当前分支的构建要求和已集成代码以本页开头及工程配置为准。

## 历史

re3 始于 2018 年春，最初用于在游戏内测试逆向得到的碰撞与物理代码，通过 DLL 将原游戏的单个函数替换成逆向实现。

项目开发一段时间后停滞约一年，于 2019 年 5 月恢复并上传 GitHub。当时 aap 已逆向约一万行，估计完整游戏约有 20 万至 25 万行代码。Fire_Head、shfil、erorcun、Nick007J 依次加入，Serge 稍后加入；2019 年夏进展迅速，此后速度放缓。

疫情初期大家居家，有更多时间参与。2020 年 4 月，项目终于生成独立可执行程序，当时代码约 18 万行。继续修复和完善后，2020 年 5 月初开始 reVC：它直接基于 re3，而不是重新从 DLL 替换函数起步。经过数月稳定开发，团队于当年 12 月认为 reVC 已完成。

旧版 README 随后记载开始了 reLCS，且当时仍在开发；此处保留其历史描述，不代表当前进度。

## 许可

原项目表示不认为自己有权为这些代码授予许可。代码仅用于教育、文档和 Mod 制作目的；不鼓励盗版或商业用途。请让衍生作品保持开源，并保留适当署名。
