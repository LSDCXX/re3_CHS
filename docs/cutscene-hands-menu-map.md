# Cutscene hands and native MenuMapIII

This integration adds animated cutscene hands and replaces the existing menu map with a native adaptation of LSDCXX/MenuMapIII.

## Sources and credits

- Hand animation/configuration: https://github.com/aap/iii_anim at `50ab6fd9f48f09ac3b28ddc81c95fc433c4ec41e`.
- Interactive map and Chinese labels: https://github.com/LSDCXX/menu-map-iii at `3c6d7e55ae0b1e6c062b1f6bbf7a253882691f0d`, based on gennariarmando/menu-map.
- Uses re3 input, streaming, RenderWare wrappers and Chinese fonts rather than ASI hooks or fixed executable addresses.

## Cutscene hands

Install the compatible iii_anim hand assets in the game's `anim` directory and its configuration as `data/cutscenehands.xml`. The assets are not bundled in this source change. Each replacement hand has independent animation state. Missing configuration, models, animations or CSHands.txd leave the original hands in use. In particular, optional textures use a single stream load, avoiding the filename loader's indefinite retry when a file is absent.

## Map

Copy `gamefiles/data/MenuMapIII.ini` to the game's `data` directory. Missing configuration uses built-in defaults. The map uses the existing 64 radar TXDs in the game archives; no extra map texture pack is required. Do not enable a separate MenuMap ASI alongside the native implementation. The upstream SkyUI compatibility setting does not apply here.

- Full-screen map, original re3 yellow footer with black instructions; the lower-right area name follows the cursor.
- Chinese/English legend and hover labels, configurable service locations, progression-based island shading.
- Open the map from the pause menu. Escape or the visible Back button returns to the pause menu; M has no map action.
- Drag, arrows or left stick pan; wheel/PgUp/PgDn or R2/L2 zoom.
- Double-click or R centres the player; L or L1 toggles the legend.
- Right-click, T or controller Square toggles a waypoint.
- Waypoint allocation handles a full radar pool and stale marker handles.

Regenerate the build project after adding the source files. The supplied build was compiled for Windows x64, librw D3D9 and OpenAL Release. Existing audio DLLs and game assets are still required.

## Validation

Full MSVC Release compilation and linking passed. Configuration, Chinese/English labels, missing INI defaults, invalid service entries, UTF-8 decoding and map coordinate/zoom tests passed. All 64 radar TXDs from the user's game were parsed using librw. Hand resource checks verified independent clump clones and changing bone matrices earlier in this integration.

The final full-screen/footer adjustment has been compiled but has not been visually verified in a running game. In-game input and appearance should still be checked.
