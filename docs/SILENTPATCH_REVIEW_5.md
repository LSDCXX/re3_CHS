# SilentPatch III: taxi extras, detached limbs, helicopter bounds and UI

Compared with the SilentPatch III `dev` changelog and implementation on
2026-09-29. These are native re3 changes guarded by `FIX_BUGS` where applicable;
they do not load the SilentPatch binary.

| Item | Result |
| --- | --- |
| Traffic taxis without roof signs | Clear requested vehicle extras when creating a model that has no extras, so a stale garage variation cannot affect the following taxi. |
| Detached limb LOD | Preserve the source atomic's cloned render callback; do not replace it with the default callback. |
| Helicopter screen-edge clipping | Give Catalina's and police helicopters a helicopter-sized bounding sphere/box instead of the pedestrian bounds. |
| Low brightness save/load | Encode the 0–511 slider in the existing one-byte settings field and decode both stock and new values, following SilentPatch's compatibility heuristic. |
| Text boxes with backgrounds | Scale horizontal and vertical padding for the normal and Chinese font paths. |

Already present before this review: radar position and disc dimensions, trace
blip size, font shadows, garage/rampage text placement, menu confirmation text,
credits, mission title timing and subtitle placement scale with resolution.

The Purple Nines fix is also already represented by `CGangs::Initialise` resetting
all gang model overrides on new-game initialization (`FIX_BUGS`). Save loading
then deliberately restores the saved gang state. This prevents cross-game state
leakage but does not silently rewrite an already affected save; such saves need
separate repair after confirming their mission state.

Source: [SilentPatch III changelog](https://github.com/CookiePLMonster/SilentPatch/blob/dev/CHANGELOG-III.md),
[2026 update and hotfix](https://silentsblog.com/2026/07/31/silentpatch-2026-update/).
Attribution and license: [SilentPatch-LICENSE.txt](SilentPatch-LICENSE.txt).

Release compilation and diff checks are recorded separately; no in-game visual
or save-state regression test has been performed for this review.
