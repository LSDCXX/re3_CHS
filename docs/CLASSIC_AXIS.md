# Classic Axis native integration

Reference: [gennariarmando/classic-axis](https://github.com/gennariarmando/classic-axis),
commit `74156ccc3c4f4f610fef5908d75c38f7f535c3c3`.
This implements its third-person aiming and movement behavior through re3's own
camera, animation and weapon APIs. It does not load the ASI or use executable-address patches.

## Controls

- Select **Standard Controls**. Hold the existing target button (normally right mouse)
  to aim; use the existing fire and movement bindings to shoot and strafe.
- **Free Cam remains independent**: enabled, the camera can orbit the player during
  ordinary movement; disabled, the original camera-relative Standard movement remains.
  Both settings support third-person aiming.
- M16 aiming stays in third person. AK47/M16 aiming smoothly narrows the field of view.
  Sniper rifle and rocket launcher keep their native first-person weapon modes.
- **C** toggles stationary crouch and supports firing supported guns while crouched.
  Sprint, jump, vehicle entry, loss of player control and incompatible weapon firing
  cancel the owned crouch. **Left Alt** limits movement to walking.
- Weapon cycling is suppressed while aiming; empty-ammo switching still runs.
- The third-person crosshair appears while manually aiming. Optional auto aim uses
  re3's existing target acquisition, switching, line-of-sight rules and target marker.

## Configuration

The optional `data/ClassicAxisIII.ini` is read from the game directory on startup.
The shipped defaults also work without this file. Restart after editing it.
The source distribution stores the file at `gamefiles/data/ClassicAxisIII.ini`.

Only the options listed in the shipped file are implemented. `WalkKey` supports
`LALT` or `NULL`; `CrouchKey` supports one letter or `NULL`. The reticle and aiming
ray are fixed at the exact screen centre; old `CameraCrosshairMultX/Y` settings
are ignored, including those in previously installed configuration files.

## Camera and crouch-fire corrections (revision 2)

Walking and jumping use a centred follow camera. Jumping retains the same camera
context but cannot activate aiming or a crosshair. Aiming blends the camera to the
right shoulder, placing the character left of the centred reticle, independently
of rifle FOV zoom. The old two-pixel HUD offset is also disabled for this mode.

Crouch firing no longer depends on crossing keyframes in the shared crouch pose.
The unmodified weapon data's animation-loop period schedules each shot: AK47 and
M16 repeat while held, and shotgun shots retain their pumping interval. Shots use
the native `CWeapon::Fire` path; native ammo, damage, sounds and timed reloads remain.
Releasing fire stops shots and holding fire resumes after the native reload ends.

This native adaptation keeps re3's input bindings and sensitivity settings. It does
not reproduce the ASI's GInput hooks, forced control-setting overrides, optional
SA/Stories marker artwork or mouse-hover health triangle.

## Building

The new `src/core/ClassicAxis.cpp` and header are automatically discovered when
generating a fresh Premake or CMake project. Regenerate an existing Visual Studio
project, or add the two files to it, before compiling. No game animation or texture
replacement is required.

## Validation

- Full Windows x64 / librw D3D9 / OpenAL release compilation and linking passed
  using the local re3 dependency libraries and Windows resource.
- 336 checks against the production module with engine doubles passed: control-mode
  gating, Free Cam independence, aiming/release, crouch transitions, context cleanup,
  NPC/global weapon isolation, weapon exclusions, walk speed, camera zoom recovery,
  centred walking/jumping, shoulder placement and repeated fire/release/reload
  at 16, 33 and 67 ms frame steps for AK47, M16 and shotgun.
- Map changes already present in the local source were preserved.
- No in-game input, animation or collision playtest has been performed. Test both
  Free Cam settings, crouch shooting/reloading, movement around walls, vehicle entry,
  death/restart, and returning to Classic Controls before publishing a release.

## One-handed aiming movement (revision 3)

Colt45 and Uzi retain normal walking/jogging input while aiming, including sustained
fire. Left Alt still explicitly selects walking and crouching remains stationary.
Two-handed weapons retain their aiming walk limit. This revision was prepared only
in the delivery workspace and was not written back to D:\re3.

Revision 3 also uses the native shot scheduler for standing aimed fire, retaining
held-fire input across finished animation poses instead of relying on keyframe
crossings. Native timed reloads still block shots; standing handguns/rifles retain
their reload animations. The flamethrower uses the existing two-handed rifle pose
for continuous emission instead of the shotgun pumping animation, without changing
its damage, emission cadence or fire offset. Regression checks cover standing and
crouched aim/fire/release/reload for Colt45, Uzi, shotgun, AK47, M16 and flamethrower
at 16, 33 and 67 ms frame steps.

## Crouch-aim-fire input entry (revision 4)

The weapon-input path now explicitly enters the Axis attack state when fire is
pressed while aiming. It no longer waits for the legacy SetAttack entry to permit
the transition from crouched PED_AIM_GUN before the new firing handler can run.
Both locked-target and manual-aim input branches use this entry. The existing shot
deadline prevents the ped tick and input tick from firing twice in one frame.

The input-order regression begins from idle, crouches, holds aim for 30 frames,
then presses and holds fire; it never assigns PED_ATTACK in the test. All six
supported guns are checked with Free Cam enabled and disabled, including release
and loss of player control. Tests use engine doubles, not an in-game playtest.

## Original standing flamethrower animation (revision 5)

The standing flamethrower override introduced in revision 3 is removed. Standing
aim and firing now use the original weapon.dat animation IDs and animation timing.
Crouched firing retains its crouch pose, and the input-entry and held-fire fixes
remain in place. This revision does not write back to the local D:\re3 checkout.

## Controller lock-on (revision 6)

Standard Controls enables lock-on when re3 detects active controller input,
independently of ForceAutoAim. Hold aim and press a native target-switch button
(L2/R2 in the internal pad layout) to acquire a target; subsequent presses switch
between targets. Moving the look stick releases the lock. Switching to mouse/keyboard
returns to manual aim unless ForceAutoAim is enabled; mouse movement also releases
an existing lock. Native range/visibility checks and target selection remain in use.
Free Cam does not control whether this feature is enabled. Classic Controls is unchanged.
The external plugin's special pad Mode 4 is not available in re3's native layouts.
Controller crouch binding is not added in this revision. No physical-pad playtest.

## Aim-button acquisition and controller menu navigation (revision 7)

Controller aim now acquires a target immediately without needing a target-switch
button. Entry after a jump also works when aim was already held. Neutral look input
retains the target; manual look releases it without immediate automatic reacquisition.
Target-switch buttons still cycle or reacquire; releasing and pressing aim reacquires.
This supersedes revision 6's requirement to press a target-switch button first.

PC menus use Circle (Xbox B) for Back. The map accepts B to return to the pause menu,
and Start resumes gameplay directly through the normal frontend shutdown path,
including on the map. The unloaded-game guard remains in place.

## Mission retry and GTA IV-style Standard controller layout (revision 8)

References:
- ZeptoBST/re3 master 645829bb62678d4f42c7ae36ebdf0c5b4cd6a99d:
  https://github.com/ZeptoBST/re3
- Silent's GInput III official package, Setup 5 in GAME CONTROLS FULL LIST.txt:
  https://silentsblog.com/mods/gta-iii/#ginput
- Classic Axis controller Mode 4 aiming behavior:
  https://github.com/gennariarmando/classic-axis

MISSION_REPLAY was already enabled in this checkout, using the same mobile retry
implementation as ZeptoBST. This revision retains that implementation and adds
snapshot validity checks, initial menu selection, a Chinese/English retry prompt
without requiring a new GXT file, and cancellation via B/Escape/Start. A snapshot
uses the existing ninth save slot, separate from the eight manual slots. Failed
writes cannot offer the previous snapshot. Midmission cutscenes preserve the
mission-start snapshot. A successful retry load restores validity after script Init.
Original mission eligibility restrictions remain (including intro/side activities).

Standard Controls with active controller input now uses the Setup 5 gameplay
layout regardless of the saved legacy pad layout index; Classic Controls and
keyboard/mouse retain their existing bindings. No GInput ASI is installed.

On foot: left stick movement, right stick view, LT aim (partial for free aim,
full for lock-on), RT fire, D-pad left/right weapons, A sprint, X jump, Y enter,
RB centre view, right-stick click look behind. While fully locked, horizontal
right-stick flicks switch targets rather than dropping the lock. LS toggles
crouch (native Axis extension), without changing the vehicle horn binding.
B attacks with melee weapons. Pickups retain re3's automatic collection behavior.

In vehicles: left stick steering, RT analog throttle, LT analog brake/reverse,
A handbrake, Y exit, LB fire, LS horn, RS native sub-mission/hydraulics action,
D-pad left/right previous/next radio, right stick turret; hold RB to use right
stick for looking left/right/behind. Sniper aiming uses right stick and zoom uses
left-stick up/down. Back cycles camera and Start toggles pause. Menus keep A
confirm/B back; map B returns to the pause menu and Start resumes gameplay.

This is a native gameplay layout implementation, not GInput's binary plugin or
its optional Steam overlay, SIXAXIS, replay-hold or MP3-track-hold extensions.
Existing four-layout controller illustrations are legacy configuration screens;
they do not describe the Standard override. The bindings above are authoritative.

Validation: full x64 D3D9/OpenAL compile/link; 379 Axis state/camera/fire tests,
74 production Pad method tests using raw controller states, and 11 production
retry/save tests. These use engine doubles, not physical-controller or mission
playtests. No writeback to D:\re3 or the game directory.

## Updated vehicle look controls and manual aim override (revision 9)

GInput's official ReadMe 1.03 changelog supersedes its older full-controls table:
LB looks left, RB looks right, and both look behind. Standard Controls now uses
those defaults; vehicle fire is assigned to B so looking left cannot also shoot.
This B fire assignment is the native integration choice; the cited changelog only
specifies the updated observation buttons. RT/LT remain analog throttle/brake.
The right stick retains turret control without requiring an observation modifier.

At the user's request, moving the view stick while aiming now releases a target
and selects manual aim for the remainder of the LT hold. Returning the stick to
neutral or varying LT between half/full pressure does not silently reacquire.
Release LT and aim again to rearm lock-on. Input uses the existing look deadzones.
LS crouch, menu/map B back and Start resume remain intact. No SilentPatch changes.
This supersedes revision 8's right-stick target-switch behavior.

Validation: 381 Axis engine-double checks and 86 real Pad method checks; the 11
unchanged retry checks remain included. Full native build, no gameplay playtest.

## Custom walk binding (revision 10)

WalkKey was incorrectly parsed as a boolean: every value except LALT disabled it.
The parser now accepts case-insensitive letters, digits, SPACE, TAB, CAPSLOCK,
ENTER, BACKSPACE, LALT/RALT/ALT, LSHIFT/RSHIFT/SHIFT and LCTRL/RCTRL/CTRL.
LCONTROL/RCONTROL/CONTROL aliases are accepted. NULL explicitly disables walking;
unknown/empty names fall back to LALT. The configured key is polled continuously,
so holding it caps walking and releasing it restores normal movement. Walking
wins when the same key is also bound to sprint. Restart the game after INI edits.
Classic Controls and inactive Axis contexts retain their original sprint behavior.

Example: WalkKey = LCTRL, or WalkKey = E. Preserve the user's existing INI when
updating just the executable. Validation: 419 Axis and 87 production Pad checks;
11 unchanged retry checks remain included. Full build; no gameplay playtest.

## D-pad target cycling and automatic next target (revision 11)

While holding controller lock-on, D-pad left/right cycles targets through the
native directional selection routine. Outside aiming the same buttons continue
to cycle weapons. Right-stick input still selects manual aim until LT is released.

When a locked ped dies, the Standard controller aim path selects the nearest
eligible living ped by horizontal distance from the player, within the current
weapon range. It excludes the player, dead/zero-health peds, peds in vehicles,
followers, unseen/occluded peds, and peds too close for a stable lock. Selection
uses native visibility and target-break checks. If no valid target exists, it
clears the dead target. Death replacement does not run in manual aim or with LT
released, and held fire can continue against the newly acquired target.

Validation: 433 Axis checks (including the actual production nearest-target
selection method), 87 production Pad checks; 11 unchanged retry checks included.
Full build, no physical-controller/gameplay test. No local installation writeback.

## Contextual control hints (revision 12)

Standard Controls action-token hints now use the current native layout instead of
the four legacy pad tables: LT aim, RT fire/throttle, LT brake, A handbrake,
B vehicle fire, LB/RB vehicle observation, D-pad weapon/target/radio selection,
and the correct sticks for aiming/zoom. Existing button icons and PlayStation
button names are reused when that controller presentation is selected.

Help messages retain their original action tokens until drawing, allowing visible
help to follow controller/keyboard input switches without restarting its timer.
Display duration still uses expanded text length. Literal button names embedded
in third-party GXT prose cannot be inferred/remapped; action-token text is updated.
This change does not add a separate tutorial for LS crouch or custom walking.
Validation: 22 production hint-label checks, full build; existing gameplay checks
are unchanged. No in-game visual test or writeback to the installed game.

## Garage camera reticle (revision 13)

Apply the Standard Controls aim-only PC reticle rule to garage cameras that
report Using3rdPersonMouseCam, including fixed garage views. These views are
not Classic Axis aiming cameras, so their legacy crosshair is suppressed.
Normal follow-camera free aim, native first-person scope reticles and Classic
Controls keep their existing behavior. Full compile/link checked; no game test.

## Chinese control hints (revision 14)

Chinese uses device-specific plain button names instead of sprite tokens that
the Chinese renderer skips. Standard action remapping and live input/language
selection remain in effect; other languages retain button icons.
Full compile/link checked; no in-game visual test.

## Chinese button icons (revision 15)

Supersedes revision 14's text fallback. Chinese now draws the same button sprites
selected by controller type as other languages, and includes their width in
wrapping and alignment. Adjacent color/button tokens are handled individually.
