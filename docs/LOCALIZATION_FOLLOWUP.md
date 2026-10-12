# SilentPatch localization and input follow-up

The default fixes-enabled build uses the Windows user's measurement preference
for both script unit checks and distance statistics. The selected interface
language continues controlling the translation, independently of units.

`General/Units` in the game's existing INI accepts `-1` (system, default), `0`
(metric) or `1` (imperial), following SilentPatch's convention. An unavailable
locale query falls back to metric. Existing precise conversion constants remain
in use. Non-Windows builds use the metric fallback.

French and German remain selectable languages. In the fixes-enabled branch,
language/keyboard selection no longer enables the regional content flags, disables
violent content or enables the prostitute restriction. This covers Windows,
GLFW, ordinary menu language reloads and PS2 menu initialization. GTA III thus
uses its normal menu logo in those languages. Original regional behavior is
retained in the fixes-disabled alternatives.

Windows document-folder lookup uses `SHGetFolderPathA` in both Windows backends,
with bounded suffix concatenation and the existing `data` failure fallback.
The default local `userfiles` preference is unchanged. No direct registry query
is needed for this feature, including the Vice City document-folder variants.

The previous hand-edited `-0.8/+0.8` mouse multipliers were reverted to the
original per-camera `-2.5/4` or `-3/4` values. The original stored vertical default
is restored too. Following SilentPatch VC's Y-sensitivity patch, all mouse pitch
paths use the horizontal sensitivity field when `FIX_BUGS` is enabled. Original
vertical-field behavior remains available with fixes disabled; stored settings
and camera layout are preserved.

GTA III script images now explicitly apply bilinear filtering and restore the
previous render state. Images and solid rectangles use scaled/centered X and
scaled Y in both pre-fade and post-fade paths. Vice City already had these helpers.

Reference: CookiePLMonster/SilentPatch `dev`, revision
`74976544adc196fa664831ddc59778e684cf6af0`, `Localization::IsMetric_LocaleBased`
and VC Y-axis sensitivity hooks. Attribution: `SilentPatch-LICENSE.txt`.

Validation: default x64 D3D9/OpenAL Release builds, fixes-disabled Release builds
with `USE_MY_DOCUMENTS` enabled, six extracted-production-helper configurations
per game, regional-assignment guard checks and existing pad/legacy-health tests.
The helper tests cover automatic/forced units, failed locale queries, pitch
sensitivity selection, script scaling/filter restoration, redirected documents,
oversized paths and local-folder fallback. No full in-game visual pass is claimed.
