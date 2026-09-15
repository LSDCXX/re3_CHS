# re3

[简体中文](README.md) | **English**

## Integrated features and fixes

- **Animated cutscene fingers**: native aap/iii_anim adaptation with independent animation state per hand; missing compatible assets retain the original hands. III hand assets must be installed separately: see [asset instructions](docs/cutscene-hands-menu-map.md).
- **Fullscreen map**: adapt LSDCXX/MenuMapIII with the yellow footer, upper-centre legend, lower-right area label above the footer, hover labels, refined font sizes/background/shadows, and corrected player heading and Back button.
- **Map navigation**: enter through the pause menu; M no longer opens or closes it. Escape, Back or B returns one level; Start resumes play. Left stick pans, LT/RT zoom, X/Square toggles waypoints, LB/L1 toggles the legend.
- **Classic Axis**: shoulder aiming, centred reticle and crouch fire under Standard Controls, with independent Free Cam. Fix offset walking cameras, jump/garage reticles, held-fire and crouch-fire timing, crouch-then-aim firing, sustained aimed movement and custom walk bindings. Standing flamethrower fire uses the original animation.
- **GInput Setup 5-style controller layout**: LT aim, RT fire, A sprint, X jump, Y enter vehicle, LS crouch. Vehicles use RT throttle, LT brake/reverse, A handbrake, B fire, LB/RB look left/right and both to look behind.
- **Lock-on and free aim**: controller target locking, D-pad target cycling and nearest eligible replacement after a target dies. Moving the right stick selects free aim until LT is released.
- **Chinese controller icons**: contextual hints follow the active device and selected controller type. Fix missing Chinese button icons and account for sprite widths during wrapping and alignment.
- **Mission retry**: retain and harden the existing mobile retry implementation with snapshot validity checks, Chinese/English prompts and cancellation. Prevent failed writes from using stale snapshots and retain the mission-start snapshot.

See [Classic Axis documentation](docs/CLASSIC_AXIS.md) for configuration, revisions and validation limits; later revisions supersede earlier behavior. This update does not systematically port the latest SilentPatch, and the unverified explosion-audio report is not listed as fixed.

Clone this fork with:

```sh
git clone --recursive --branch master https://github.com/LSDCXX/re3_CHS.git re3
```

**The previous README is preserved below.** Historical upstream download links, platform testing and C++ restrictions do not describe all new fork code. Builds and isolated checks do not replace full gameplay testing.

Credits: [iii_anim](https://github.com/aap/iii_anim), [MenuMapIII](https://github.com/LSDCXX/menu-map-iii), [original MenuMap](https://github.com/gennariarmando/menu-map), [Classic Axis](https://github.com/gennariarmando/classic-axis), [GInput III](https://silentsblog.com/mods/gta-iii/#ginput), [ZeptoBST/re3](https://github.com/ZeptoBST/re3). Chinese terminology follows the [GTAMODX translation](https://gtamodx.com/mods/BmUYhBABACsQ). Existing credits, screenshots, links, configuration, build instructions, history and licensing statements are retained; see the [unchanged previous README](docs/README_LEGACY.md).


---

<img src="https://github.com/hezkore/re3/blob/master/res/images/logo_1024.png?raw=true" alt="re3 logo" width="200">

## About this fork of re3
This fork fixes some issues I've had with the original re3.\
They're currently only applied to GTA 3.

* The camera now has the same vertical and horizontal speed.
* The mouse is generally slower, letting you adjust the mouse sensitivity at a finer-granularity level.
* Particles are no longer affected by framerate.\
_(via a dirty fixed rate hack)_

## Intro

## Dynamic Chinese fonts

On Windows, Chinese text can now be rasterized on demand from installed system fonts or local TTF/TTC/OTF files. Configure it in `re3.ini`:

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

`TextRenderer=3` uses DirectWrite (default), `2` uses the compatible GDI renderer, and `1` keeps the legacy `MODELS/CHINESE.TXD` plus `data/Chinese.dat` path. Dynamic modes cache glyphs in atlas pages as characters are encountered, so they are not limited to the glyphs bundled in the static texture.

When Chinese is loaded, the normal glyphs referenced by `CHINESE.GXT` are pre-rendered while the loading screen is active, regardless of whether the font system or GXT data finishes loading first. This makes the first load slightly longer, but prevents new cutscene subtitles from stalling gameplay. GXT-external characters in one string share a single atlas lock/upload. Chinese HUD glyphs are displayed 10% larger. As in reVC, glyph quads are grouped by atlas texture and submitted in separate shadow/main batches with linear filtering; Latin text keeps the original game's sizing and rendering path.

In this repository you'll find the fully reversed source code for GTA III ([master](https://github.com/hezkore/re3/tree/master/) branch) and GTA VC ([miami](https://github.com/GTAmodding/re3/tree/miami/) branch).

It has been tested and works on Windows, Linux, MacOS and FreeBSD, on x86, amd64, arm and arm64.\
Rendering is handled either by original RenderWare (D3D8)
or the reimplementation [librw](https://github.com/aap/librw) (D3D9, OpenGL 2.1 or above, OpenGL ES 2.0 or above).\
Audio is done with MSS (using dlls from original GTA) or OpenAL.

The project has also been ported to the [Nintendo Switch](https://github.com/AGraber/re3-nx/),
[Playstation Vita](https://github.com/Rinnegatamante/re3) and
[Nintendo Wii U](https://github.com/GaryOderNichts/re3-wiiu/).

We cannot build for PS2 or Xbox yet. If you're interested in doing so, get in touch with us.

## Installation

- re3 requires PC game assets to work, so you **must** own [a copy of GTA III](https://store.steampowered.com/app/12100/Grand_Theft_Auto_III/).
- Build re3 or download the latest build:
  - [Windows D3D9 MSS 32bit](https://nightly.link/GTAmodding/re3/workflows/re3_msvc_x86/master/re3_Release_win-x86-librw_d3d9-mss.zip)
  - [Windows D3D9 64bit](https://nightly.link/GTAmodding/re3/workflows/re3_msvc_amd64/master/re3_Release_win-amd64-librw_d3d9-oal.zip)
  - [Windows OpenGL 64bit](https://nightly.link/GTAmodding/re3/workflows/re3_msvc_amd64/master/re3_Release_win-amd64-librw_gl3_glfw-oal.zip)
  - [Linux 64bit](https://nightly.link/GTAmodding/re3/workflows/build-cmake-conan/master/ubuntu-18.04-gl3.zip)
  - [MacOS 64bit x86-64](https://nightly.link/GTAmodding/re3/workflows/build-cmake-conan/master/macos-latest-gl3.zip)
- Extract the downloaded zip over your GTA 3 directory and run re3. The zip includes the binary, updated and additional gamefiles and in case of OpenAL the required dlls.

## Screenshots

![re3 2021-02-11 22-57-03-23](https://user-images.githubusercontent.com/1521437/107704085-fbdabd00-6cbc-11eb-8406-8951a80ccb16.png)
![re3 2021-02-11 22-43-44-98](https://user-images.githubusercontent.com/1521437/107703339-cbdeea00-6cbb-11eb-8f0b-07daa105d470.png)
![re3 2021-02-11 22-46-33-76](https://user-images.githubusercontent.com/1521437/107703343-cd101700-6cbb-11eb-9ccd-012cb90524b7.png)
![re3 2021-02-11 22-50-29-54](https://user-images.githubusercontent.com/1521437/107703348-d00b0780-6cbb-11eb-8afd-054249c2b95e.png)

## Improvements

We have implemented a number of changes and improvements to the original game.
They can be configured in `core/config.h`.
Some of them can be toggled at runtime, some cannot.

* Fixed a lot of smaller and bigger bugs
* User files (saves and settings) stored in GTA root directory
* Settings stored in re3.ini file instead of gta3.set
* Debug menu to do and change various things (Ctrl-M to open)
* Debug camera (Ctrl-B to toggle)
* Rotatable camera
* XInput controller support (Windows)
* No loading screens between islands ("map memory usage" in menu)
* Skinned ped support (models from Xbox or Mobile)
* Rendering
  * Widescreen support (properly scaled HUD, Menu and FOV)
  * PS2 MatFX (vehicle reflections)
  * PS2 alpha test (better rendering of transparency)
  * PS2 particles
  * Xbox vehicle rendering
  * Xbox world lightmap rendering (needs Xbox map)
  * Xbox ped rim light
  * Xbox screen rain droplets
  * More customizable colourfilter
* Menu
  * Map
  * More options
  * Controller configuration menu
  * ...
* Can load DFFs and TXDs from other platforms, possibly with a performance penalty
* ...

## To-Do

The following things would be nice to have/do:

* Fix physics for high FPS
* Improve performance on lower end devices, especially the OpenGL layer on the Raspberry Pi (if you have experience with this, please get in touch)
* Compare code with PS2 code (tedious, no good decompiler)
* [PS2 port](https://github.com/GTAmodding/re3/wiki/PS2-port)
* Xbox port (not quite as important)
* reverse remaining unused/debug functions
* compare CodeWarrior build with original binary for more accurate code (very tedious)

## Modding

Asset modifications (models, texture, handling, script, ...) should work the same way as with original GTA for the most part.

CLEO scripts work with [CLEO Redux](https://github.com/cleolibrary/CLEO-Redux).

Mods that make changes to the code (dll/asi, limit adjusters) will *not* work.
Some things these mods do are already implemented in re3 (much of SkyGFX, GInput, SilentPatch, Widescreen fix),
others can easily be achieved (increasing limis, see `config.h`),
others will simply have to be rewritten and integrated into the code directly.
Sorry for the inconvenience.

## Building from Source

When using premake, you may want to point GTA_III_RE_DIR environment variable to GTA3 root folder if you want the executable to be moved there via post-build script.

Clone the repository with `git clone --recursive https://github.com/GTAmodding/re3.git`. Then `cd re3` into the cloned repository.

<details><summary>Linux Premake</summary>

For Linux using premake, proceed: [Building on Linux](https://github.com/GTAmodding/re3/wiki/Building-on-Linux)

</details>

<details><summary>Linux Conan</summary>

Install python and conan, and then run build.
```
conan export vendor/librw librw/master@
mkdir build
cd build
conan install .. re3/master@ -if build -o re3:audio=openal -o librw:platform=gl3 -o librw:gl3_gfxlib=glfw --build missing -s re3:build_type=RelWithDebInfo -s librw:build_type=RelWithDebInfo
conan build .. -if build -bf build -pf package
```
</details>

<details><summary>MacOS Premake</summary>

For MacOS using premake, proceed: [Building on MacOS](https://github.com/GTAmodding/re3/wiki/Building-on-MacOS)

</details>

<details><summary>FreeBSD</summary>

For FreeBSD using premake, proceed: [Building on FreeBSD](https://github.com/GTAmodding/re3/wiki/Building-on-FreeBSD)

</details>

<details><summary>Windows</summary>

Assuming you have Visual Studio 2015/2017/2019:
- Run one of the `premake-vsXXXX.cmd` variants on root folder.
- Open build/re3.sln with Visual Studio and compile the solution.

Microsoft recently discontinued its downloads of the DX9 SDK. You can download an archived version here: https://archive.org/details/dxsdk_jun10

**If you choose OpenAL on Windows** You must read [Running OpenAL build on Windows](https://github.com/GTAmodding/re3/wiki/Running-OpenAL-build-on-Windows).
</details>

> :information_source: premake has an `--with-lto` option if you want the project to be compiled with Link Time Optimization.

> :information_source: There are various settings in [config.h](https://github.com/GTAmodding/re3/tree/master/src/core/config.h), you may want to take a look there.

> :information_source: re3 uses completely homebrew RenderWare-replacement rendering engine; [librw](https://github.com/aap/librw/). librw comes as submodule of re3, but you also can use LIBRW enviorenment variable to specify path to your own librw.

If you feel the need, you can also use CodeWarrior 7 to compile re3 using the supplied codewarrior/re3.mcp project - this requires the original RW33 libraries, and the DX8 SDK. The build is unstable compared to the MSVC builds though, and is mostly meant to serve as a reference.

## Contributing
As long as it's not linux/cross-platform skeleton/compatibility layer, all of the code on the repo that's not behind a preprocessor condition(like FIX_BUGS) are **completely** reversed code from original binaries.

We **don't** accept custom codes, as long as it's not wrapped via preprocessor conditions, or it's linux/cross-platform skeleton/compatibility layer.

We accept only these kinds of PRs;

- A new feature that exists in at least one of the GTAs (if it wasn't in III/VC then it doesn't have to be decompilation)
- Game, UI or UX bug fixes (if it's a fix to original code, it should be behind FIX_BUGS)
- Platform-specific and/or unused code that's not been reversed yet
- Makes reversed code more understandable/accurate, as in "which code would produce this assembly".
- A new cross-platform skeleton/compatibility layer, or improvements to them
- Translation fixes, for languages original game supported
- Code that increase maintainability

We have a [Coding Style](https://github.com/GTAmodding/re3/blob/master/CODING_STYLE.md) document that isn't followed or enforced very well.

Do not use features from C++11 or later.


## History

re3 was started sometime in the spring of 2018,
initially as a way to test reversed collision and physics code
inside the game.
This was done by replacing single functions of the game
with their reversed counterparts using a dll.

After a bit of work the project lay dormant for about a year
and was picked up again and pushed to github in May 2019.
At the time I (aap) had reversed around 10k lines of code and estimated
the final game to have around 200-250k.
Others quickly joined the effort (Fire_Head, shfil, erorcun and Nick007J
in time order, and Serge a bit later) and we made very quick progress
throughout the summer of 2019
after which the pace slowed down a bit.

Due to everyone staying home during the start of the Corona pandemic
everybody had a lot of time to work on re3 again and
we finally got a standalone exe in April 2020 (around 180k lines by then).

After the initial excitement and fixing and polishing the code further,
reVC was started in early May 2020 by starting from re3 code,
not by starting from scratch replacing functions with a dll.
After a few months of mostly steady progress we considered reVC
finished in December.

Since then we have started reLCS, which is currently work in progress.


## License

We don't feel like we're in a position to give this code a license.\
The code should only be used for educational, documentation and modding purposes.\
We do not encourage piracy or commercial use.\
Please keep derivate work open source and give proper credit.
